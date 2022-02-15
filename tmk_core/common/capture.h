#ifndef CAPTURE_H
#define CAPTURE_H

// capture.h should be included in every compile unit when defining 'fixed register'
//#include "config.h"

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is based on Soarers's work on sctrace
 * https://deskthority.net/viewtopic.php?t=4567
 */


// timer overflow count
extern volatile uint8_t tovf;

// circular buffer queue for capture data
extern volatile uint8_t *iqhead;
extern volatile uint8_t *iqtail;

void capture_init(void);

// print capture data in iqueue
void print_capture(void);
void print_capture_all(void);

#if defined(__AVR__)
// pin to be captured
#define CAPTURE_PIN         PIND

// pin state stored in capture()
#define CAPTURE_PIN_STORED  _SFR_IO8(0x01)

// store 4-byte capture records in SRAM iqueue area(0x0100-0x02FF)
// capture() takes around 3us
static inline void capture(void) __attribute__ ((always_inline));
static inline void capture(void) {
    asm volatile (
        "push   r0"                     "\n\t"
        "in     r0,     %[pin]"         "\n\t"
        "out    1,      r0"             "\n\t"  /* Store pin state into unused IO address 0x01 */
        "push   r26"                    "\n\t"
        "push   r27"                    "\n\t"
        "lds    r26,    %[iqh]"         "\n\t"
        "lds    r27,    %[iqh]+1"       "\n\t"
        "st     X+,     r0"             "\n\t"  /* [0]: pin state */
        "lds    r0,     %[tcl]"         "\n\t"
        "st     X+,     r0"             "\n\t"  /* [1]: timer low */
        "lds    r0,     %[tch]"         "\n\t"
        "st     X+,     r0"             "\n\t"  /* [2]: timer high */

        "push   r1"                     "\n\t"
        "in     r1,     __SREG__"       "\n\t"

        "tst    r0"                     "\n\t"  /* check timer rollover before timer OVF ISR */
        "lds    r0,     %[tov]"         "\n\t"
        "brne   2f"                     "\n\t"
        "sbic   %[tfr], 0"              "\n\t"
        "inc    r0"                     "\n\t"  /* tov+1 if (timerH == 0) and (TOV1 of TIFR1 == 1) */
    "2:"
        "st     X+,     r0"             "\n\t"  /* [3]: timer overflow */

        "sts    %[iqh], r26"            "\n\t"  /* if (iqhead == 0x0300) iqhead = 0x0100; */
        "cpi    r27,    3"              "\n\t"
        "brne   1f"                     "\n\t"
        "ldi    r27,    1"              "\n\t"
    "1:"                                "\n\t"
        "sts    %[iqh]+1, r27"          "\n\t"

        "out    __SREG__, r1"           "\n\t"
        "pop    r1"                     "\n\t"
        "pop    r27"                    "\n\t"
        "pop    r26"                    "\n\t"
        "pop    r0"                     "\n\t"
        : [iqh] "+X" (iqhead)
        : [pin] "I" (_SFR_IO_ADDR(CAPTURE_PIN)),
          [tcl] "X" (TCNT1L),
          [tch] "X" (TCNT1H),
          [tov] "X" (tovf),
          [tfr] "I" (_SFR_IO_ADDR(TIFR1))
    );
}
#else
#   error "This microcontroller doesn't support, currently only for AVR"
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
