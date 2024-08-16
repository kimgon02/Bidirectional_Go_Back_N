#include <stdio.h>
#include <stdlib.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
float timeout = 30.0;//timeout 시간 -> 30 seconds
#define BUFFSIZE 1024 // A의 sequence number size 설정
int A_base;//A의 base number를 저장하는 변수
int A_next_seq_num;//A의 next sequence number를 저장
int A_expected_seq_num;//A의 expected sequence number를 저장
int A_ACK_state;//A의 ACKstate를 저장하여, acknum을 999로 설정할지 말지에 대해 결정한다.
int A_ACK_num;//piggy back 구현을 위해, 수신한 ack num을 저장
int A_window_size = 8;
int A_timer_on = 0;//해당 값이 1인 경우, A의 timer가 작동 중임을 의미
int B_base;//B의 base number를 저장
int B_next_seq_num;//B의 next sequence number를 저장
int B_window_size = 8;
int B_ACK_num;//piggy back 구현을 위해, 수신한 ack num을 저장
int B_ACK_state;//B의 ACKstate를 저장하여, acknum을 999로 설정할지 말지에 대해 결정
int B_expected_seq_num;//B의 전송 받아야 할 sequence number를 저장
int B_timer_on = 0;//해당 값이 1인 경우, B의 timer가 작동 중임을 의미
struct pkt A_sender_window[BUFFSIZE];//A의 buffer
struct pkt B_sender_window[BUFFSIZE];//B의 buffer
unsigned short checksum16(struct pkt packet) {//16 bit checksum generator
    int checksum = 0;
    for (int i = 0; i < 10; i++) {
        checksum+=(int)packet.payload[i * 2] * 256 + (int)packet.payload[2 * i + 1];//모든 payload를 누적하여 더한다.
    }
    checksum += packet.acknum;//acknum, seqnum, checksum을 누적하여 더한다.
    checksum += packet.seqnum;
    checksum += packet.checksum;
    while (checksum >= 65536) {//더한 결과가 16bit을 초과하는 경우, 16bit 이하가 될때 까지 wrap up을 수행한다.
        int upper_16bit = (checksum / 65536) * 65536;
        checksum -= upper_16bit;
        checksum += upper_16bit / 65536;
    }
    return ~((unsigned short)(checksum));//생성된 16bit값에 1의 보수를 취하여 반환한다.
}


/* called from layer 5, passed the data to be sent to other side */

//layer 5에서 전송 요청이 들어온 경우 (rdt_send(data))
A_output(message)
struct msg message;
{
    if (A_next_seq_num < A_base + A_window_size) {//Buffer full이 아닌경우
        if (A_ACK_state == 0) A_ACK_num = 999;
        printf("A_output : send ACK(ack=%d)\n", A_ACK_num);
        printf("A_output : send packet(seq = %d) : ", A_next_seq_num);
        for (int i = 0; i < 20; i++) putchar(message.data[i]);
        putchar('\n');
        //==========make packet============
        for (int i = 0; i < 20; i++) {
            A_sender_window[A_next_seq_num % BUFFSIZE].payload[i] = message.data[i];//message를 packet payload로 복사
        }
        A_sender_window[A_next_seq_num % BUFFSIZE].seqnum = A_next_seq_num;//seq num 저장
        A_sender_window[A_next_seq_num%BUFFSIZE].checksum = 0;
        A_sender_window[A_next_seq_num%BUFFSIZE].acknum = A_ACK_num;//ACK num 저장
        A_sender_window[A_next_seq_num%BUFFSIZE].checksum = checksum16(A_sender_window[A_next_seq_num%BUFFSIZE]);//checksum 저장
        //==================================
        tolayer3(0, A_sender_window[A_next_seq_num%BUFFSIZE]);
        A_ACK_state = 0;
        if (A_base == A_next_seq_num) {
            starttimer(0, timeout);//start timer / initial condition 의 경우 timer set
            printf("A_output : start timer.\n");
            A_timer_on = 1;
        }
        A_next_seq_num++;//sequence number 갱신
    }
    else printf("A_output : Buffer is full. Drop the message\n");
    return 0;
}

B_output(message)
struct msg message;
{
    if (B_next_seq_num < B_base + B_window_size) {//Buffer full이 아닌 경우
        if (B_ACK_state == 0) B_ACK_num = 999;//보낼 ack 가 없는 경우
        printf("B_output : send ACK(ack=%d)\n", B_ACK_num);
        printf("B_output : send packet(seq = %d) : ", B_next_seq_num);
        for (int i = 0; i < 20; i++) putchar(message.data[i]);
        putchar('\n');
        /*==========make packet============*/
        for (int i = 0; i < 20; i++) {
            B_sender_window[B_next_seq_num % BUFFSIZE].payload[i] = message.data[i];//message를 packet payload로 복사
        }
        B_sender_window[B_next_seq_num % BUFFSIZE].seqnum = B_next_seq_num;//seq num 저장
        B_sender_window[B_next_seq_num % BUFFSIZE].checksum = 0;
        B_sender_window[B_next_seq_num % BUFFSIZE].acknum = B_ACK_num;//ACK num 저장
        B_sender_window[B_next_seq_num % BUFFSIZE].checksum = checksum16(B_sender_window[B_next_seq_num%BUFFSIZE]);//checksum 저장
        /*==================================*/
        if (B_base == B_next_seq_num) {
            printf("B_output : start timer.\n");
            starttimer(1, timeout);//start timer / initial condition 의 경우 timer set
            B_timer_on = 1;
        }
        tolayer3(1, B_sender_window[B_next_seq_num%BUFFSIZE]);//packet 전송
        B_ACK_state = 0;
        B_next_seq_num++;//sequence number 갱신
    }
    else printf("B_output : Buffer is full. Drop the message\n");
    return 0;
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)//수신자가 packet을 받았을 경우.(rdt_rcv(rcvpkt))
struct pkt packet;
{
    if (checksum16(packet) != (unsigned short)0) {//rdt_rcv(rcvpkt)&&corrupt(rcvpkt)
        printf("A_input : \"Packet corrupted. Drop.\"\n");//Packet corrupted
        return 0;//corrupted message
    }
    if (packet.seqnum != A_expected_seq_num) {//seqnum이 matching 되지 않을 경우 가장 오래된 ACK 발송
        printf("A_input: not the expected seq. send NAK(ack = seq%d)\n", A_expected_seq_num);
    }
    else {//seqnum이 matching 되는 경우 ACK갱신 후 발송, 5계층으로 전송 및 expected seq num 갱신
        printf("A_input: recv packet(seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++) putchar(packet.payload[i]);
        putchar('\n');
        tolayer5(0, packet.payload);//message 전송
        A_ACK_num = A_expected_seq_num;
        A_ACK_state = 1;
        A_expected_seq_num++;
    }

    if (packet.acknum != 999) {//sender 입장 ack 수신   (rdt_rcv(rcvpkt) && notcorrupt(rcvpkt))
        if (A_base > packet.acknum) {//확인된 packet이 온경우, packet을 무시한다.
            printf("A_input : got NAK(ack = %d). Drop.\n", packet.acknum);
            return 0;
        }
        printf("A_input : got ACK(ack = %d)\n", packet.acknum);
        A_base = packet.acknum + 1;//base를 갱신한다.
        if (A_base == A_next_seq_num) {//모든 확인 응답이 온 경우, timer를 멈춘다(더이상의 데이터 송수신이 없으므로)
            if (A_timer_on == 1) {
                stoptimer(0);
                printf("A_input : stop timer.\n");
                A_timer_on = 0;
            }
        }
        else {//그렇지 않다면, timer를 초기화 한다.
            if (A_timer_on == 1) {
                stoptimer(0);
            }
            starttimer(0, timeout);
            printf("A_input : start timer.\n");
            if (A_timer_on == 0) A_timer_on = 1;
        }
    }
    return 0;
}

/* called when A's timer goes off */
A_timerinterrupt()//timeout
{
    starttimer(0, timeout);//timer 재시작
    for (int i = A_base; i < A_next_seq_num; i++) {
        printf("A_timerinterrupt : resend packet (seq = %d): ", A_sender_window[i].seqnum);
        for (int j = 0; j < 20; j++) putchar(A_sender_window[i].payload[j]);
        putchar('\n');
    }
    for (int i = A_base; i < A_next_seq_num; i++) {
        tolayer3(0, A_sender_window[i]);//base number부터 next_seq_num-1 까지의 sequence key들을 재전송
    }
    return 0;
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
A_init()//프로그램 시작시 A에 대한 변수 초기화
{
    A_base = 1;
    A_next_seq_num = 1;
    A_expected_seq_num=1;
    A_ACK_state = 0;
    A_ACK_num=0;
    return 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)//B가 패킷을 받음 (rdt_rcv(rcvpkt))
struct pkt packet;
{
    if (checksum16(packet) != (unsigned short)0) {//corrupt packet인 경우
        printf("B_input : \"Packet corrupted. Drop.\"\n");//Packet corrupted
        return 0;
    }
    if (packet.seqnum != B_expected_seq_num) {//seqnum이 matching 되지 않을 경우 가장 오래된 ACK 발송
        printf("B_input : not the expected seq. send NAK(ack = seq%d)\n", B_expected_seq_num);
    }
    else {//seqnum이 matching 되는 경우 ACK갱신 후 발송, 5계층으로 전송 및 expected seq num 갱신
        printf("B_input : recv packet(seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++) putchar(packet.payload[i]);
        putchar('\n');
        tolayer5(1, packet.payload);
        B_ACK_num = B_expected_seq_num;
        B_ACK_state = 1;
        B_expected_seq_num++;
    }
    if (packet.acknum != 999) {//sender 입장 ack 수신
        if (B_base > packet.acknum) {//Not the expected  sequence number
            printf("B_input : got NAK(ack = %d). Drop.\n", packet.acknum);
            return 0;
        }
        printf("B_input : got ACK(ack = %d)\n", packet.acknum);
        B_base = packet.acknum + 1;//base를 갱신한다.
        if (B_base == B_next_seq_num) {
            if (B_timer_on == 1) {
                stoptimer(1);//모든 확인 응답이 온 경우 stoptimer.
                B_timer_on = 0;
                printf("B_input : stop timer.\n");
            }
        }
        else {
            if (B_timer_on == 1) {
                stoptimer(1);
            }
            starttimer(1, timeout);//그렇지 않다면, timer를 초기화한다.
            if (B_timer_on == 0) B_timer_on = 1;
            printf("B_input : start timer.\n");
        }
    }
    return 0;

}

/* called when B's timer goes off */
B_timerinterrupt()
{
    starttimer(1, timeout);//timer 재시작
    printf("B_timerinterrupt : start timer.\n");
    for (int i = B_base; i < B_next_seq_num; i++) {
        printf("B_timerinterrupt : resend packet (seq = %d): ", B_sender_window[i].seqnum);
        for (int j = 0; j < 20; j++) putchar(B_sender_window[i].payload[j]);
        putchar('\n');
    }
    for (int i = B_base; i < B_next_seq_num; i++) {
        tolayer3(1, B_sender_window[i]);//base number부터 next_seq_num-1 까지의 sequence key들을 재전송
    }
    return 0;
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
B_init()//프로그램 시작시 B에 대한 변수 초기화
{
    B_base = 1;
    B_next_seq_num = 1;
    B_expected_seq_num = 1;
    B_ACK_state = 0;
    B_ACK_num = 0;
    return 0;
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity where event occurs */
    struct pkt* pktptr;     /* ptr to packet (if any) assoc w/ this event */
    struct event* prev;
    struct event* next;
};
struct event* evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
    struct event* eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim == nsimmax)
            break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");
                for (i = 0; i < 20; i++)
                    printf("%c", msg2give.data[i]);
                printf("\n");
            }
            nsim++;
            if (eventptr->eventity == A)
                A_output(msg2give);
            else
                B_output(msg2give);
        }
        else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr);          /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
}



init()                         /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();


    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d", &nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f", &lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f", &corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f", &lambda);
    printf("Enter TRACE:");
    scanf("%d", &TRACE);

    srand(9999);              /* init random number generator */
    sum = 0.0;                /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;                    /* initialize time to 0.0 */
    generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
    double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    float x;                   /* individual students may need to change mmm */
    x = rand() / mmm;            /* x should be uniform in [0,1] */
    return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
    double x, log(), ceil();
    struct event* evptr;
    char* malloc();
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
                              /* having mean of lambda        */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


insertevent(p)
struct event* p;
{
    struct event* q, * qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

printevlist()
{
    struct event* q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
    struct event* q, * qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else {     /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB, increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

    struct event* q;
    struct event* evptr;
    char* malloc();

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
tolayer3(AorB, packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
    struct pkt* mypktptr;
    struct event* evptr, * q;
    char* malloc();
    float lastime, x, jimsrand();
    int i;


    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt*)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
            mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
     medium can not reorder, so make sure packet arrives between 1 and 10
     time units after the latest arrival time of packets
     currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();



    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

tolayer5(AorB, datasent)
int AorB;
char datasent[20];
{
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }

}