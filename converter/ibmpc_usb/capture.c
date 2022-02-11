#include <avr/interrupt.h>
#include "sendchar.h"
#include "capture.h"

// iqueue placed at SRAM 0x0100-02FF before .data section
// place this in Makeilfe: EXTRALDFLAGS += -Wl,--section-start,.data=0x800300
volatile uint8_t *iqhead = (uint8_t *)0x0100;
volatile uint8_t *iqtail = (uint8_t *)0x0100;

void capture_init(void)
{
#if 0
    // PD2-5,PD7,PB4 for Debug use for logic analyzer
    DDRD |= (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 7);
    PORTD &= ~( (1 << 2) | (1 << 3) |(1 << 4) | (1 << 5) |  (1 << 7) );
    DDRB |= (1 << 4);
    PORTB &= ~(1 << 4);
#endif

    // Timer1
    TCCR1A = 0x00;
    TCCR1B = 0x01;
    TIMSK1 = (1<<TOIE1);
}

static void print_hh(uint8_t v)
{
    sendchar((v >> 4) + ((v >> 4) < 10 ? '0' : 'A' - 10));
    sendchar((v & 0x0F) + ((v & 0x0F) < 10 ? '0' : 'A' - 10));
}

void print_capture(void)
{
    if (iqhead == iqtail) return;

    // Capture record in iqueue buffer:
    //     [0]:Pin
    //     [1]:Timer low
    //     [2]:Timer high
    //     [3]:Timer overflow

    // Print record data in hexdecimal with format below:
    // [3], [2], [1], ':', [0]
    print_hh(*(iqtail+3));
    print_hh(*(iqtail+2));
    print_hh(*(iqtail+1));
    sendchar(':');
    print_hh(*(iqtail+0));
    sendchar(' ');

    iqtail += 4;
    if (iqtail == (uint8_t *)0x0300)
        iqtail = (uint8_t *)0x0100;
}

void print_capture_all(void)
{
    while (iqhead != iqtail) {
        print_capture();
    }
}


volatile uint8_t tovf = 0;
ISR(TIMER1_OVF_vect, ISR_NAKED)
{
    asm volatile (
        "push   r0"                 "\n\t"
        "in     r0, __SREG__"       "\n\t"
        "push   r0"                 "\n\t"
        "lds    r0, %[xtra]"        "\n\t"
        "inc    r0"                 "\n\t"
        "sts    %[xtra], r0"        "\n\t"
#ifdef CAPTURE_TIMER_ROLLOVER_RECORD
        "brne   tovf_end"           "\n\t"
#else
        "pop    r0"                 "\n\t"
        "out    __SREG__, r0"       "\n\t"
        "pop    r0"                 "\n\t"
        "reti"                      "\n\t"
#endif
        : [xtra] "+X" (tovf)
        :
    );

#ifdef CAPTURE_TIMER_ROLLOVER_RECORD
    // capture record for rollover of tovf
    capture();

    asm volatile (
    "tovf_end:"                     "\n\t"
        "pop    r0"                 "\n\t"
        "out    __SREG__, r0"       "\n\t"
        "pop    r0"                 "\n\t"
        "reti"                      "\n\t"
        ::
    );
#endif
}
