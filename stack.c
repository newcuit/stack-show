#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_STACK_DEPTH         100

static inline uint64_t rdtsc(void) {
        uint32_t lo, hi;
        __asm__ __volatile__ (
                "rdtsc"
                : "=a"(lo), "=d"(hi)
                :
                :
        );
        return (uint64_t)hi << 32 | lo;
}

static volatile int stack_start;
static unsigned long stack_x_depth, stack_y_depth[MAX_STACK_DEPTH];
static double ts_record[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static double ts_record_total[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static const char *ts_record_name[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static inline int stack_set(int status)
{
  if (status) {
        stack_x_depth = 0;
        memset(stack_y_depth, 0, sizeof(stack_y_depth));
  }
        stack_start = status;
        return 0;
}

static inline int stack_begin(const char *func_name)
{
        long tick;

        if (!stack_start)
                return 0;

        tick = rdtsc();
        ts_record[stack_x_depth][stack_y_depth[stack_x_depth]] = tick;
        ts_record_name[stack_x_depth][stack_y_depth[stack_x_depth]] = func_name;
        stack_x_depth++;
        if (stack_x_depth >= MAX_STACK_DEPTH) {
                printf("Warning: stack_x_depth(%ld) > MAX_STACK_DEPTH\n", stack_x_depth);
        }
        return 0;
}

static inline int stack_end(const char *func_name)
{
        long tick;

        if (!stack_start)
                return 0;

        tick = rdtsc();
        stack_x_depth--;
        if (func_name != ts_record_name[stack_x_depth][stack_y_depth[stack_x_depth]]) {
                printf("error:%s != %s\n", func_name, ts_record_name[stack_x_depth][stack_y_depth[stack_x_depth]]);
                return 0;
        }
        ts_record[stack_x_depth][stack_y_depth[stack_x_depth]] = tick-ts_record[stack_x_depth][stack_y_depth[stack_x_depth]];
        ts_record_total[stack_x_depth][stack_y_depth[stack_x_depth]] = ts_record[stack_x_depth][stack_y_depth[stack_x_depth]];
        stack_y_depth[stack_x_depth]++;

        if (stack_y_depth[stack_x_depth] >= MAX_STACK_DEPTH) {
                printf("Warning: stack_y_depth(%ld) > MAX_STACK_DEPTH\n", stack_y_depth[stack_x_depth]);
        }
        return 0;
}

static inline int show_stack(unsigned int count)
{
        int x, y, i;

  if (count == 0) {
        return 0;
  }

        for (y = 0; y < MAX_STACK_DEPTH; y++) {
                for (x = 0; x < MAX_STACK_DEPTH; x++) {
                        if (ts_record[x][y] == 0)
                                continue;

                        for (i = 0; i < x; i++)
                                printf("\t");
                        printf("%lf: %s\n", ts_record_total[x][y]/count, ts_record_name[x][y]);
                }
        }
  memset(ts_record_total, 0, sizeof(ts_record_total));
        return 0;
}

#include <stdio.h>
#include <unistd.h>

unsigned int funcc(void)
{
  unsigned int i = 0, total = 0;

  stack_begin(__func__);
  for (;i < 20000; i++) {
        total += i;
  }
  stack_end(__func__);

  return total;
}

unsigned int funcd(void)
{
  unsigned int i = 0, total = 0;

  stack_begin(__func__);
  for (;i < 200000; i++) {
        total += i;
  }
  total = total+funcc();
  stack_end(__func__);

  return total;
}

unsigned int funcb(void)
{
  unsigned int total = 0;

  stack_begin(__func__);
  total += funcc();
  stack_end(__func__);

  return total;
}

unsigned int funca(void)
{
  unsigned int total = 0;

  stack_begin(__func__);
  total += funcb();
  total += funcc();
  total += funcd();
  stack_end(__func__);
  return total;
}

int main()
{
  int i;
  unsigned int total;

  for (i = 0; i < 4; i++) {
        stack_set(1);
        stack_begin(__func__);
        total = funca();
        stack_end(__func__);
        stack_set(0);
  }
  printf("total = %lu\n", total);
  show_stack(4);
  return 0;
}
