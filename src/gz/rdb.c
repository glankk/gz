#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>
#include <startup.h>
#include <mips.h>
#include <vr4300.h>
#include "ed.h"
#include "rdb.h"
#include "util.h"
#include "z64.h"

#define RDB_DEBUG_FAULT 0
#define RDB_DEBUG_SCHED 1
#define RDB_SWBKP_MAX   16

struct fifo_pkt
{
  uint16_t            size;
  char                data[510];
};

struct swbkp
{
  _Bool               active;
  uint32_t            addr;
  uint32_t            old_insn;
  uint32_t            new_insn;
};

static OSThread *const rdb_threads[] =
{
  &z64_thread_idle,
#if defined(RDB_DEBUG_FAULT) && RDB_DEBUG_FAULT
  &z64_thread_fault,
#endif
  &z64_thread_main,
  &z64_thread_graph,
#if defined(RDB_DEBUG_SCHED) && RDB_DEBUG_SCHED
  &z64_thread_sched,
#endif
  &z64_thread_padmgr,
  &z64_thread_audio,
  &z64_thread_dmamgr,
  &z64_thread_irqmgr,
};

static const char *const rdb_thread_name[] =
{
  "idle",
#if defined(RDB_DEBUG_FAULT) && RDB_DEBUG_FAULT
  "fault",
#endif
  "main",
  "graph",
#if defined(RDB_DEBUG_SCHED) && RDB_DEBUG_SCHED
  "sched",
#endif
  "padmgr",
  "audio",
  "dmamgr",
  "irqmgr",
};

static struct
{
  _Alignas(0x10)
  struct fifo_pkt     ipkt;
  _Alignas(0x10)
  struct fifo_pkt     opkt;
  int                 ipos;

  _Bool               noack;

  OSThread           *cthread;
  OSThread           *gthread;

  struct swbkp        swbkp[RDB_SWBKP_MAX];

  _Bool               watch_active;
  uint32_t            watch_addr;
  uint32_t            watch_length;
  uint32_t            watch_type;

  volatile _Bool      detach;

} rdb;

static volatile _Bool rdb_active;
static OSMesgQueue    rdb_fault_mq;
static OSMesg         rdb_fault_mesg[8];

static int hex_char(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static char hex_int(int d)
{
  if (d >= 0x0 && d <= 0x9)
    return d + '0';
  if (d >= 0xA && d <= 0xF)
    return d + 'a' - 10;
  return ' ';
}

static _Bool check_addr(uint32_t addr, uint32_t size)
{
  return addr >= 0x80000000 && addr < 0xC0000000 && (addr & (size - 1)) == 0;
}

static _Bool rdb_poll(void)
{
  return rdb.ipos != rdb.ipkt.size || !(ed_regs.status & ED_STATE_RXF);
}

static void rdb_flush(void)
{
  if (rdb.opkt.size == 0)
    return;
  while (ed_regs.status & (ED_STATE_TXE | ED_STATE_DMA_BUSY))
    ;
  ed_fifo_write(&rdb.opkt, 1);
  rdb.opkt.size = 0;
}

static char rdb_getc(void)
{
  if (rdb.ipos == rdb.ipkt.size) {
    while (ed_regs.status & (ED_STATE_RXF | ED_STATE_DMA_BUSY))
      ;
    ed_fifo_read(&rdb.ipkt, 1);
    rdb.ipos = 0;
  }
  return rdb.ipkt.data[rdb.ipos++];
}

static void rdb_putc(char c)
{
  if (rdb.opkt.size == sizeof(rdb.opkt.data))
    rdb_flush();
  rdb.opkt.data[rdb.opkt.size++] = c;
}

static char *rdb_getpkt(_Bool notification)
{
  const int bufsize = 128;
  char *pkt = NULL;

  /* get packet, retry on checksum error */
  _Bool retry;
  do {
    retry = 0;
    int pkt_size = 0;
    int pkt_cap = bufsize;
    pkt = malloc(pkt_cap + 1);
    if (!pkt) {
      errno = ENOMEM;
      goto err;
    }
    int tx_csum = -1;
    uint8_t rx_csum = 0;

    /* receive packet data */
    while (1) {
      char c = rdb_getc();

      /* check for packet terminator */
      if (c == '#') {
        int ci1 = hex_char(rdb_getc());
        int ci2 = hex_char(rdb_getc());
        if (ci1 != -1 && ci2 != -1)
            tx_csum = ci1 * 0x10 + ci2;
        break;
      }
      rx_csum += c;
      int rl = 1;

      /* check for escape sequence */
      if (c == '}') {
        c = rdb_getc();
        rx_csum += c;
        c ^= ' ';
      }
      /* check for rle sequence */
      else if (c == '*' && pkt_size > 0) {
        c = rdb_getc();
        rx_csum += c;
        rl = c - '\x1D';
        c = pkt[pkt_size - 1];
      }

      /* allocate and insert packet data */
      if (pkt_size + rl > pkt_cap) {
        if (pkt_size + rl > pkt_cap + bufsize)
          pkt_cap = pkt_size + rl;
        else
          pkt_cap += bufsize;
        char *new_pkt = realloc(pkt, pkt_cap + 1);
        if (!new_pkt) {
          errno = ENOMEM;
          goto err;
        }
        pkt = new_pkt;
      }
      memset(&pkt[pkt_size], c, rl);
      pkt_size += rl;
    }
    pkt[pkt_size] = 0;

    if (tx_csum == rx_csum) {
      /* checksum ok; acknowledge */
      if (!rdb.noack && !notification) {
        rdb_putc('+');
        rdb_flush();
      }
    }
    else {
      /* checksum failed; drop packet if it's a notification.
       * otherwise, request a retransmission.
       */
      if (rdb.noack || notification)
        goto err;
      rdb_putc('-');
      rdb_flush();
      /* wait for retransmission */
      while (1) {
        if (rdb_getc() == '$')
          break;
      }
      /* start over */
      free(pkt);
      retry = 1;
    }

  } while (retry);

  return pkt;

err:
  if (pkt)
    free(pkt);
  return NULL;
}

static int rdb_putpkt(_Bool notification, const char *fmt, ...)
{
  /* allocate and format package */
  va_list ap;
  va_start(ap, fmt);
  int pkt_size = vsnprintf(NULL, 0, fmt, ap) + 1;
  va_end(ap);

  char *pkt = malloc(pkt_size);
  if (!pkt) {
    errno = ENOMEM;
    goto err;
  }
  va_start(ap, fmt);
  vsnprintf(pkt, pkt_size, fmt, ap);
  va_end(ap);

  /* send packet, retry on checksum error */
  _Bool retry;
  do {
    retry = 0;
    char *p = pkt;
    uint8_t csum = 0;

    /* send packet intro */
    rdb_putc(notification ? '%' : '$');

    /* send packet data */
    while (*p) {
      char c = *p++;

      /* escape bad characters */
      if (c == '#' || c == '$' || c == '*' || c == '}') {
        rdb_putc('}');
        csum += '}';
        c ^= ' ';
      }

      rdb_putc(c);
      csum += c;
    }

    /* send terminator and checksum */
    rdb_putc('#');
    rdb_putc(hex_int(csum / 0x10));
    rdb_putc(hex_int(csum % 0x10));
    rdb_flush();

    /* check ack */
  if (!rdb.noack && !notification) {
      char c = rdb_getc();
      if (c == '-')
        retry = 1;
      else if (c != '+') {
        /* malformed ack */
        goto err;
      }
    }

  } while (retry);

  free(pkt);
  return 0;

err:
  if (pkt)
    free(pkt);
  return -1;
}

static uint64_t rdb_get_reg(OSThread *thread, int reg_idx)
{
  __OSThreadContext *c = &thread->context;
  uint64_t *f = (void*)c->fp64;
  switch (reg_idx) {
    case 0x01:  return c->at;
    case 0x02:  return c->v0;
    case 0x03:  return c->v1;
    case 0x04:  return c->a0;
    case 0x05:  return c->a1;
    case 0x06:  return c->a2;
    case 0x07:  return c->a3;
    case 0x08:  return c->t0;
    case 0x09:  return c->t1;
    case 0x0A:  return c->t2;
    case 0x0B:  return c->t3;
    case 0x0C:  return c->t4;
    case 0x0D:  return c->t5;
    case 0x0E:  return c->t6;
    case 0x0F:  return c->t7;
    case 0x10:  return c->s0;
    case 0x11:  return c->s1;
    case 0x12:  return c->s2;
    case 0x13:  return c->s4;
    case 0x14:  return c->s5;
    case 0x15:  return c->s5;
    case 0x16:  return c->s6;
    case 0x17:  return c->s7;
    case 0x18:  return c->t8;
    case 0x19:  return c->t9;
    case 0x1C:  return c->gp;
    case 0x1D:  return c->sp;
    case 0x1E:  return c->s8;
    case 0x1F:  return c->ra;
    case 0x20:  return c->sr;
    case 0x21:  return c->lo;
    case 0x22:  return c->hi;
    case 0x23:  return c->badvaddr;
    case 0x24:  return c->cause;
    case 0x25:  return c->pc;
    case 0x26:  return f[0];
    case 0x28:  return f[1];
    case 0x2A:  return f[2];
    case 0x2C:  return f[3];
    case 0x2E:  return f[4];
    case 0x30:  return f[5];
    case 0x32:  return f[6];
    case 0x34:  return f[7];
    case 0x36:  return f[8];
    case 0x38:  return f[9];
    case 0x3A:  return f[10];
    case 0x3C:  return f[11];
    case 0x3E:  return f[12];
    case 0x40:  return f[13];
    case 0x42:  return f[14];
    case 0x44:  return f[15];
    case 0x46:  return c->fpcsr;
  }
  return 0;
}

static void rdb_set_reg(OSThread *thread, int reg_idx, uint64_t value)
{
  __OSThreadContext *c = &thread->context;
  uint64_t *f = (void*)c->fp64;
  switch (reg_idx) {
    case 0x01:  c->at       = value;  break;
    case 0x02:  c->v0       = value;  break;
    case 0x03:  c->v1       = value;  break;
    case 0x04:  c->a0       = value;  break;
    case 0x05:  c->a1       = value;  break;
    case 0x06:  c->a2       = value;  break;
    case 0x07:  c->a3       = value;  break;
    case 0x08:  c->t0       = value;  break;
    case 0x09:  c->t1       = value;  break;
    case 0x0A:  c->t2       = value;  break;
    case 0x0B:  c->t3       = value;  break;
    case 0x0C:  c->t4       = value;  break;
    case 0x0D:  c->t5       = value;  break;
    case 0x0E:  c->t6       = value;  break;
    case 0x0F:  c->t7       = value;  break;
    case 0x10:  c->s0       = value;  break;
    case 0x11:  c->s1       = value;  break;
    case 0x12:  c->s2       = value;  break;
    case 0x13:  c->s4       = value;  break;
    case 0x14:  c->s5       = value;  break;
    case 0x15:  c->s5       = value;  break;
    case 0x16:  c->s6       = value;  break;
    case 0x17:  c->s7       = value;  break;
    case 0x18:  c->t8       = value;  break;
    case 0x19:  c->t9       = value;  break;
    case 0x1C:  c->gp       = value;  break;
    case 0x1D:  c->sp       = value;  break;
    case 0x1E:  c->s8       = value;  break;
    case 0x1F:  c->ra       = value;  break;
    case 0x20:  c->sr       = value;  break;
    case 0x21:  c->lo       = value;  break;
    case 0x22:  c->hi       = value;  break;
    case 0x23:  c->badvaddr = value;  break;
    case 0x24:  c->cause    = value;  break;
    case 0x25:  c->pc       = value;  break;
    case 0x26:  f[0]        = value;  break;
    case 0x28:  f[1]        = value;  break;
    case 0x2A:  f[2]        = value;  break;
    case 0x2C:  f[3]        = value;  break;
    case 0x2E:  f[4]        = value;  break;
    case 0x30:  f[5]        = value;  break;
    case 0x32:  f[6]        = value;  break;
    case 0x34:  f[7]        = value;  break;
    case 0x36:  f[8]        = value;  break;
    case 0x38:  f[9]        = value;  break;
    case 0x3A:  f[10]       = value;  break;
    case 0x3C:  f[11]       = value;  break;
    case 0x3E:  f[12]       = value;  break;
    case 0x40:  f[13]       = value;  break;
    case 0x42:  f[14]       = value;  break;
    case 0x44:  f[15]       = value;  break;
    case 0x46:  c->fpcsr    = value;  break;
  }
}

static _Bool rdb_set_bkp(struct swbkp *bkp, uint32_t addr)
{
  if (bkp->active)
    return bkp->addr == addr;
  if (!check_addr(addr, 4))
    return 0;
  uint32_t *p = (void*)addr;
  bkp->active = 1;
  bkp->addr = addr;
  bkp->old_insn = *p;
  bkp->new_insn = MIPS_TEQ(MIPS_R0, MIPS_R0, 0);
  *p = bkp->new_insn;
  __asm__ volatile ("cache 0x19, 0(%0);"
                    "cache 0x10, 0(%0);" :: "r"(p));
  return 1;
}

static _Bool rdb_clear_bkp(struct swbkp *bkp)
{
  if (!bkp->active)
    return 1;
  uint32_t *p = (void*)bkp->addr;
  bkp->active = 0;
  if (*p == bkp->new_insn) {
    *p = bkp->old_insn;
    __asm__ volatile ("cache 0x19, 0(%0);"
                      "cache 0x10, 0(%0);" :: "r"(p));
  }
  return 1;
}

static void rdb_enable_watch(void)
{
  uint32_t watchlo;
  if (rdb.watch_active)
    watchlo = (rdb.watch_addr & 0x1FFFFFF8) | (rdb.watch_type & 3);
  else
    watchlo = 0;
  __asm__ volatile ("mtc0  %0, $18;" :: "r"(watchlo));
}

static void rdb_disable_watch(void)
{
  __asm__ volatile ("mtc0  $zero, $18;");
}

static int rdb_nthreads(void)
{
  return sizeof(rdb_threads) / sizeof(*rdb_threads);
}

static OSThread *rdb_thread_by_id(OSId thread_id)
{
  for (int i = 0; i < rdb_nthreads(); ++i)
    if (rdb_threads[i]->id == thread_id)
      return rdb_threads[i];
  return NULL;
}

static void rdb_startall(void)
{
  for (int i = 0; i < rdb_nthreads(); ++i)
    z64_osStartThread(rdb_threads[i]);
}

static void rdb_stopall(void)
{
  for (int i = 0; i < rdb_nthreads(); ++i)
    z64_osStopThread(rdb_threads[i]);
}

__attribute__((optimize(0)))
static OSThread *rdb_continue(void)
{
  rdb_enable_watch();
  rdb_startall();
  z64_osRecvMesg(&rdb_fault_mq, NULL, OS_MESG_BLOCK);
  rdb_stopall();
  rdb_disable_watch();
  return z64_osGetCurrFaultedThread();
}

static OSThread *rdb_step(OSThread *thread)
{
  struct swbkp step_bkp[2];
  step_bkp[0].active = 0;
  step_bkp[1].active = 0;

  struct vr4300_insn insn;
  uint32_t pc = thread->context.pc;

  /* set breakpoints */
  if (check_addr(pc, 4) && vr4300_decode_insn(*(uint32_t*)pc, &insn)) {
    switch (insn.opcode) {
      /* 0 operand branch */
      case VR4300_OP_BC1F:
      case VR4300_OP_BC1FL:
      case VR4300_OP_BC1T:
      case VR4300_OP_BC1TL:
        rdb_set_bkp(&step_bkp[0], pc + 4 + insn.opnd_value[0]);
        rdb_set_bkp(&step_bkp[1], pc + 8);
        break;
      /* 1 operand branch */
      case VR4300_OP_BGEZ:
      case VR4300_OP_BGEZAL:
      case VR4300_OP_BGEZALL:
      case VR4300_OP_BGEZL:
      case VR4300_OP_BGTZ:
      case VR4300_OP_BGTZL:
      case VR4300_OP_BLEZ:
      case VR4300_OP_BLEZL:
      case VR4300_OP_BLTZ:
      case VR4300_OP_BLTZAL:
      case VR4300_OP_BLTZALL:
      case VR4300_OP_BLTZL:
        rdb_set_bkp(&step_bkp[0], pc + 4 + insn.opnd_value[1]);
        rdb_set_bkp(&step_bkp[1], pc + 8);
        break;
      /* 2 operand branch */
      case VR4300_OP_BEQ:
      case VR4300_OP_BEQL:
      case VR4300_OP_BNE:
      case VR4300_OP_BNEL:
        rdb_set_bkp(&step_bkp[0], pc + 4 + insn.opnd_value[2]);
        rdb_set_bkp(&step_bkp[1], pc + 8);
        break;
      /* jump */
      case VR4300_OP_J:
      case VR4300_OP_JAL:
        rdb_set_bkp(&step_bkp[0], (pc & 0xF0000000) | insn.opnd_value[0]);
        break;
      /* register jump */
      case VR4300_OP_JALR:
        rdb_set_bkp(&step_bkp[0], rdb_get_reg(thread, insn.opnd_value[1]));
        break;
      case VR4300_OP_JR:
        rdb_set_bkp(&step_bkp[0], rdb_get_reg(thread, insn.opnd_value[0]));
        break;
      /* other */
      default:
        rdb_set_bkp(&step_bkp[0], pc + 4);
        break;
    }
  }
  else
    rdb_set_bkp(&step_bkp[0], pc + 4);

  /* run */
  thread = rdb_continue();

  /* clear breakpoints */
  rdb_clear_bkp(&step_bkp[0]);
  rdb_clear_bkp(&step_bkp[1]);

  return thread;
}

static void rdb_stop_reply(OSThread *thread)
{
  uint32_t sig;
  char rs[32] = "";
  char ts[32] = "";
  switch ((thread->context.cause >> 2) & 0x1F) {
    case 0:   /* int */
      sig = SIGINT;   break;
    case 10:  /* ri */
      sig = SIGILL;   break;
    case 15:  /* fpe */
      sig = SIGFPE;   break;
    case 2:   /* tlbl */
    case 3:   /* tlbs */
    case 4:   /* adel */
    case 5:   /* ades */
      sig = SIGSEGV;  break;
    case 6:   /* ibe */
    case 7:   /* dbe */
      sig = SIGBUS;   break;
    case 13:  /* tr */
      if (check_addr(thread->context.pc, 4) &&
          *(uint32_t*)thread->context.pc == RDB_INTR_OP)
      {
        thread->context.pc += 4;
        if (rdb.detach) {
          rdb_putpkt(0, "X0F");
          return;
        }
        sig = SIGINT; break;
      }
      sig = SIGTRAP;  break;
    case 8:   /* sys */
    case 9:   /* bp */
      sig = SIGTRAP;  break;
    case 23:  /* watch */
      if (rdb.watch_active) {
        switch (rdb.watch_type) {
          case 1: sprintf(rs, "watch:%08"  PRIx32 ";", rdb.watch_addr); break;
          case 2: sprintf(rs, "rwatch:%08" PRIx32 ";", rdb.watch_addr); break;
          case 3: sprintf(rs, "awatch:%08" PRIx32 ";", rdb.watch_addr); break;
        }
      }
      sig = SIGTRAP;  break;
    default:
      sig = SIGTRAP;  break;
  }
  if (thread->id != 0)
    sprintf(ts, "thread:%" PRIx32 ";", thread->id);
  rdb_putpkt(0, "T%02" PRIx32 "%s25:%016" PRIx64 ";%s",
             sig, ts, rdb_get_reg(thread, 0x25), rs);
}

static void rdb_main(void *arg)
{
  init_gp();
  memset(&rdb, 0, sizeof(rdb));
  ed_open();
#if !(defined(RDB_DEBUG_FAULT) && RDB_DEBUG_FAULT)
  z64_osStopThread(&z64_thread_fault);
#endif
  rdb_stopall();
  rdb_disable_watch();

  static _Bool fault_event_set;
  if (fault_event_set) {
    while (z64_osRecvMesg(&rdb_fault_mq, NULL, OS_MESG_NOBLOCK) == 0)
      ;
  }
  else {
    z64_osCreateMesgQueue(&rdb_fault_mq, rdb_fault_mesg,
                          sizeof(rdb_fault_mesg) / sizeof(*rdb_fault_mesg));
    z64_osSetEventMesg(OS_EVENT_CPU_BREAK, &rdb_fault_mq,
                       (OSMesg)OS_EVENT_CPU_BREAK);
    z64_osSetEventMesg(OS_EVENT_FAULT, &rdb_fault_mq,
                       (OSMesg)OS_EVENT_FAULT);
    fault_event_set = 1;
  }

  rdb.cthread = rdb_threads[0];
  rdb.gthread = rdb_threads[0];

  while (!rdb.detach) {
    if (!rdb_poll())
      continue;

    char *pkt = NULL;
    if (rdb_getc() == '$')
      pkt = rdb_getpkt(0);
    if (!pkt)
      continue;

    uint32_t addr;
    uint32_t length;
    uint64_t value;
    char cmd;
    char type;
    int n;

    if (strncmp(pkt, "qSupported", 10) == 0)
      rdb_putpkt(0, "QStartNoAckMode+");
    else if (strcmp(pkt, "QStartNoAckMode") == 0) {
      rdb_putpkt(0, "OK");
      rdb.noack = 1;
    }
    else if (strcmp(pkt, "qAttached") == 0)
      rdb_putpkt(0, "1");
    else if (strcmp(pkt, "qfThreadInfo") == 0) {
      char *o_pkt = malloc(rdb_nthreads() * 9);
      if (o_pkt) {
        char *p = o_pkt;
        for (int i = 0; i < rdb_nthreads(); ++i) {
          if (i > 0)
            *p++ = ',';
          p += sprintf(p, "%" PRIx32, rdb_threads[i]->id);
        }
        rdb_putpkt(0, "m%s", o_pkt);
        free(o_pkt);
      }
      else
        rdb_putpkt(0, "E00");
    }
    else if (strcmp(pkt, "qsThreadInfo") == 0)
      rdb_putpkt(0, "l");
    else if (sscanf(pkt, "qThreadExtraInfo,%" SCNx32 "%n", &addr, &n) == 1
             && pkt[n] == 0)
    {
      int thread_idx = -1;
      for (int i = 0; i < rdb_nthreads(); ++i)
        if (rdb_threads[i]->id == addr) {
          thread_idx = i;
          break;
        }
      if (thread_idx == -1)
        rdb_putpkt(0, "");
      else {
        const char *s = rdb_thread_name[thread_idx];
        int l = strlen(s);
        char *o_pkt = malloc(l * 2 + 1);
        if (o_pkt) {
          char *p = o_pkt;
          for (int i = 0; i < l; ++i)
            p += sprintf(p, "%02x", s[i]);
          rdb_putpkt(0, "%s", o_pkt);
          free(o_pkt);
        }
        else
          rdb_putpkt(0, "E00");
      }
    }
    else if (strcmp(pkt, "qC") == 0)
      rdb_putpkt(0, "QC%" PRIx32, rdb.cthread->id);
    else if (sscanf(pkt, "H%[cg]%" SCNx32 "%n", &cmd, &addr, &n) == 2
             && pkt[n] == 0)
    {
      if (addr == 0)
        rdb_putpkt(0, "OK");
      else {
        OSThread *thread = rdb_thread_by_id(addr);
        if (thread) {
          if (cmd == 'c')
            rdb.cthread = thread;
          else if (cmd == 'g')
            rdb.gthread = thread;
          rdb_putpkt(0, "OK");
        }
        else
          rdb_putpkt(0, "E00");
      }
    }
    else if (sscanf(pkt, "T%" SCNx32 "%n", &addr, &n) == 1
             && pkt[n] == 0)
    {
      OSThread *thread = rdb_thread_by_id(addr);
      if (thread)
        rdb_putpkt(0, "OK");
      else
        rdb_putpkt(0, "E00");
    }
    else if (strcmp(pkt, "?") == 0)
      rdb_stop_reply(rdb.cthread);
    else if (strcmp(pkt, "g") == 0) {
      char s[0x49 * 0x10 + 1];
      char *p = s;
      for (int i = 0; i < 0x48; ++i)
        p += sprintf(p, "%016" PRIx64, rdb_get_reg(rdb.gthread, i));
      rdb_putpkt(0, "%s", s);
    }
    else if (strncmp(pkt, "G", 1) == 0) {
      char *p = &pkt[1];
      for (int i = 0; i < 0x48; ++i) {
        if (sscanf(p, "%16" SCNx64 "%n", &value, &n) == 1) {
          rdb_set_reg(rdb.gthread, i, value);
          p += n;
        }
        else
          break;
      }
      rdb_putpkt(0, "OK");
    }
    else if (sscanf(pkt, "p%" SCNx32 "%n", &addr, &n) == 1 && pkt[n] == 0) {
      rdb_putpkt(0, "%016" PRIx64, rdb_get_reg(rdb.gthread, addr));
    }
    else if (sscanf(pkt, "P%" SCNx32 "=%16" SCNx64 "%n",
                    &addr, &value, &n) == 2
             && pkt[n] == 0)
    {
      rdb_set_reg(rdb.gthread, addr, value);
      rdb_putpkt(0, "OK");
    }
    else if (sscanf(pkt, "m%" SCNx32 ",%" SCNx32 "%n", &addr, &length, &n) == 2
             && pkt[n] == 0)
    {
      if (check_addr(addr, 1)) {
        char *o_pkt = malloc(length * 2 + 1);
        if (o_pkt) {
          char *p = o_pkt;
          *p = 0;
          for (int i = 0; i < length; ++i) {
            if (check_addr(addr, 1))
              p += sprintf(p, "%02" PRIx8, *(uint8_t*)addr++);
            else
              break;
          }
          rdb_putpkt(0, "%s", o_pkt);
          free(o_pkt);
        }
        else
          rdb_putpkt(0, "E00");
      }
      else
        rdb_putpkt(0, "E00");
    }
    else if (sscanf(pkt, "M%" SCNx32 ",%" SCNx32 "%n", &addr, &length, &n) == 2
             && pkt[n] == ':')
    {
      if (check_addr(addr, 1)) {
        char *p = &pkt[n + 1];
        uint32_t i;
        for (i = 0; i < length; ++i) {
          uint32_t value;
          if (check_addr(addr, 1)
              && sscanf(p, "%2" SCNx32 "%n", &value, &n) == 1 && n == 2)
          {
            *(uint8_t*)addr++ = value;
            p += n;
          }
          else
            break;
        }
        if (i == length && *p == 0)
          rdb_putpkt(0, "OK");
        else
          rdb_putpkt(0, "E00");
      }
      else
        rdb_putpkt(0, "E00");
    }
    else if (sscanf(pkt, "%1[Zz]%1[0234],%" SCNx32 ",%" SCNx32 "%n",
                    &cmd, &type, &addr, &length, &n) == 4
             && pkt[n] == 0)
    {
      if (type == '0') {
        if (length != 4)
          rdb_putpkt(0, "E00");
        else {
          _Bool r = 0;
          struct swbkp *b = NULL;
          for (int i = 0; i < RDB_SWBKP_MAX; ++i) {
            struct swbkp *bi = &rdb.swbkp[i];
            if (bi->active && bi->addr == addr) {
              b = bi;
              break;
            }
            else if (!b && !bi->active)
              b = bi;
          }
          if (b) {
            if (cmd == 'Z')
              r = rdb_set_bkp(b, addr);
            else if (cmd == 'z')
              r = rdb_clear_bkp(b);
          }
          if (r)
            rdb_putpkt(0, "OK");
          else
            rdb_putpkt(0, "E00");
        }
      }
      else {
        uint32_t t = type - '1';
        if (cmd == 'Z') {
          if (rdb.watch_active) {
            if (rdb.watch_addr == addr &&
                rdb.watch_length == length && rdb.watch_type == t)
            {
              rdb_putpkt(0, "OK");
            }
            else
              rdb_putpkt(0, "E00");
          }
          else {
            rdb.watch_active = 1;
            rdb.watch_addr = addr;
            rdb.watch_length = length;
            rdb.watch_type = t;
            rdb_putpkt(0, "OK");
          }
        }
        else if (cmd == 'z') {
          if (rdb.watch_active && rdb.watch_addr == addr &&
              rdb.watch_length == length && rdb.watch_type == t)
          {
            rdb.watch_active = 0;
          }
          rdb_putpkt(0, "OK");
        }
      }
    }
    else if (strcmp(pkt, "c") == 0 ||
             (sscanf(pkt, "C%" SCNx32 "%n", &addr, &n) == 1 && pkt[n] == 0))
    {
      rdb.cthread = rdb_continue();
      rdb.gthread = rdb.cthread;
      rdb_stop_reply(rdb.cthread);
    }
    else if (strcmp(pkt, "s") == 0 ||
             (sscanf(pkt, "S%" SCNx32 "%n", &addr, &n) == 1 && pkt[n] == 0))
    {
      rdb.cthread = rdb_step(rdb.cthread);
      rdb.gthread = rdb.cthread;
      rdb_stop_reply(rdb.cthread);
    }
    else if (strcmp(pkt, "D") == 0) {
      rdb_putpkt(0, "OK");
      rdb.detach = 1;
    }
    else
      rdb_putpkt(0, "");

    free(pkt);
  }

  for (int i = 0; i < RDB_SWBKP_MAX; ++i)
    rdb_clear_bkp(&rdb.swbkp[i]);
  rdb_startall();
#if !(defined(RDB_DEBUG_FAULT) && RDB_DEBUG_FAULT)
  z64_osStartThread(&z64_thread_fault);
#endif
  rdb_active = 0;
}

void rdb_start(void)
{
  static OSThread rdb_thread;
  static __attribute__((section(".stack"))) _Alignas(8)
  char rdb_stack[0x1000];

  _Bool ie = set_int(0);
  if (!rdb_active) {
    rdb_active = 1;
    z64_osCreateThread(&rdb_thread, 0, rdb_main, NULL,
                       &rdb_stack[sizeof(rdb_stack)], OS_PRIORITY_RMON);
    z64_osStartThread(&rdb_thread);
  }
  set_int(ie);
}

void rdb_stop(void)
{
  _Bool ie = set_int(0);
  if (rdb_active) {
    rdb.detach = 1;
    rdb_interrupt();
  }
  set_int(ie);
}

_Bool rdb_check(void)
{
  return rdb_active;
}

#include <vr4300.c>
