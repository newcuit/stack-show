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

static volatile int record_begin;
static unsigned long x_index, x_starty[MAX_STACK_DEPTH], y_index;
static double timestamp[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static double timestamp_total[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static const char *timestamp_name[MAX_STACK_DEPTH][MAX_STACK_DEPTH];
static inline int stack_set(int status)
{
        if (status) {
                x_index = 0;
                y_index = 0;
                memset(x_starty, 0, sizeof(x_starty));
        }
        record_begin = status;
        return 0;
}

static inline int stack_begin(const char *func_name)
{
        long tick;

        if (!record_begin) {
                return 0;
        }

        tick = rdtsc();
        x_starty[x_index] = y_index;
        timestamp[x_index][x_starty[x_index]] = tick;
        timestamp_name[x_index][x_starty[x_index]] = func_name;
        
        x_index++;
        if (x_index >= MAX_STACK_DEPTH) {
                printf("Warning: x_index(%ld) > MAX_STACK_DEPTH\n", x_index);
        }
        return 0;
}

static inline int stack_end(const char *func_name)
{
        long tick;

        if (!record_begin) {
                return 0;
        }

        x_index--;
        tick = rdtsc();
        if (func_name != timestamp_name[x_index][x_starty[x_index]]) {
                printf("error:%s != %s\n", func_name, timestamp_name[x_index][x_starty[x_index]]);
                return 0;
        }
        timestamp[x_index][x_starty[x_index]] = tick - timestamp[x_index][x_starty[x_index]];
        timestamp_total[x_index][x_starty[x_index]] += timestamp[x_index][x_starty[x_index]];
        
        y_index++;
        if (y_index >= MAX_STACK_DEPTH) {
                printf("Warning: y_index(%ld) > MAX_STACK_DEPTH\n", y_index);
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
                        if (timestamp_total[x][y] == 0)
                                continue;

                        for (i = 0; i < x; i++)
                                printf("\t");
                        printf("%lf: %s\n", timestamp_total[x][y]/count, timestamp_name[x][y]);
                }
        }
        memset(timestamp_total, 0, sizeof(timestamp_total));
        return 0;
}


// test code
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

  for (i = 0; i < 4; i++) { // 4 times
        stack_set(1);  // start record
        stack_begin(__func__); // trace point start
        total = funca();
        stack_end(__func__);   // trace point end
        stack_set(0);  // stop record
  }
  printf("total = %lu\n", total);
  show_stack(4);
  return 0;
}
