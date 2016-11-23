#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<getopt.h>
#include<time.h>
#include<string.h>
#include<unistd.h>
#include<sys/time.h>
#include"my402list.h"
#include<pthread.h>
#include<errno.h>
#include<signal.h>
#include<sys/stat.h>

sigset_t set;
struct timeval tv;
time_t ct;
float lambda = 1.0, mu = 0.35, B = 10.0, r = 1.5;
long int n = 20, P = 3, cur=0;
int opt;
FILE *f = NULL;
My402List *Q1,*Q2;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
long int dropped = 0,pcp=0,pct=0,tdropped=0,pserv = 0;
pthread_t pg, tb, s1, s2, sig;
double interarr = 0,servt = 0, totq1=0,totq2=0,tots1=0,tots2=0, totavg = 0, totavg2 = 0;

typedef struct packet
{
	char name[12];
	float a_time;
	int ntokens;
	float s_time;
	double tstrt,tq1,tq1l,tq2,tq2l;
}packet; 

void finish()
{
	struct timeval tv1;
	double total;
	double sd;
	gettimeofday(&tv1, NULL);
	total = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
	printf("%012.3fms: emulation ends\nStatistics:\n\n",(double)total);
	printf("\taverage packet inter-arrival time = %.6gs\n",(interarr/pcp)/1000.0);
	printf("\taverage packet service time = %.6gs\n\n",(servt/pserv)/1000.0);
	printf("\taverage number of packets in Q1 = %.6g\n",totq1/total);
	printf("\taverage number of packets in Q2 = %.6g\n",totq2/total);
	printf("\taverage number of packets at S1 = %.6g\n",tots1/total);
	printf("\taverage number of packets at S2 = %.6g\n\n",tots2/total);
	printf("\taverage time a packet spent in system = %.6gs\n",(double)(totavg/pserv)/1000.0);
	sd = (totavg2/pserv)-((totavg*totavg)/(pserv*pserv));
	sd = sqrt(sd);
	printf("\tstandard deviation for time spent in system = %.6gs\n\n",sd/1000.0);
	printf("\ttoken drop probability = %.6g\n",(double)tdropped/pct);
        printf("\tpacket drop probability = %.6g\n",(double)dropped/pcp);
	//exit(0);
}

void clean_gen()
{
}

void *Packet_Gen()
{
	struct timeval tv1;
	double t,tprev=0,bk1,bk2;
	gettimeofday(&tv1, NULL);
	bk1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
	pthread_cleanup_push(clean_gen, NULL);
	long int count = 0,count2 = 0;
	packet *p;
	char buff[30],buf[30];
	long int i=0,j=0;
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );
	while( count2 < n )
	{
		p = (packet*) malloc( sizeof(packet) );
		if( f == stdin )
		{
			p->a_time = (1.0/lambda) > 10? 10.00: 1/lambda;
			p->ntokens = P;
			p->s_time = (1.0/mu)>10? 10.0: 1/mu;
		}
		else if( fgets( buff, sizeof(buff), f ) != NULL )
		{
			count = 0;
			if( (buff[i] == ' ' || buff[i] == '\t') && i == 0 )
			{
				fputs( "Found leading space\n", stderr );
			}
			count = 0;i=0;j=0;
			while( i<strlen(buff) )
			{
				if( buff[i] == ' ' || buff[i] == '\t' || buff[i] == '\n')
				{
					buf[j] = '\0';
					if( count == 0 )
					{
						p->a_time = atof( buf )/1000;
						count++;j=0;
					}
					else if( count == 1 )
					{
						p->ntokens = atoi( buf );
						count++;j=0;
					}
					else if( count == 2 )
					{
						p->s_time = atof( buf )/1000;
						j=0;count++;
					}
					while( (buff[i] == ' ' || buff[i] == '\t') && i<strlen(buff))
					{
						i++;
						if( count == 3 )
						{
							fputs( "Found Trailing space\n", stderr );
							exit(0);
						}
					}
				}
				buf[j++] = buff[i++];
			}
			if( count < 3 )
			{
				fprintf( stderr, "Missing fields in input file\n" );
				exit(0);
			}
		}
		gettimeofday(&tv1, NULL);
		bk2 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		usleep( p->a_time*1000000 - (bk2-bk1)*1000 );
		gettimeofday(&tv1, NULL);
		bk1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		sprintf(p->name, "p%ld", ++pcp);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL); 
		gettimeofday(&tv1, NULL);
		t = ((tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0);
		interarr+=t-tprev;
		p->tstrt = t;
		if( p -> ntokens <= B )
		{
		printf( "%012.3fms: %s arrives, needs %d tokens, inter-arrival time = %.3f\n",(double)t, p->name, p->ntokens, (double)t-tprev );
			count2++;
			gettimeofday(&tv1, NULL);
			t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
			p->tq1 = t;
		
			pthread_mutex_lock( &m );
			My402ListAppend( Q1, p );
			printf("%012.3fms: %s enters Q1\n",(double)t,p->name);
			pthread_mutex_unlock( &m );
		}
		else
		{
		pthread_mutex_lock( &m );
	printf( "%012.3fms: %s arrives, needs %d tokens, inter-arrival time = %.3f, dropped\n",(double)t, p->name, p->ntokens, (double)t-tprev );
		dropped++;
		count2++;
		pthread_mutex_unlock( &m );
		}
		tprev = t;
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_testcancel();
		p = NULL;
	}
	pthread_cleanup_pop(0);
	return 0;
}

void clean_tb()
{

}
void *token_bucket_filter()
{
	double t,bk1,bk2;
	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	bk1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
	pthread_cleanup_push(clean_tb,NULL);
	long int ps = 0, stop=0;
	char tname[12];
	My402ListElem *temp;
	packet *temp1;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
	while( !stop )
	{
		pthread_mutex_lock( &m );
		if( My402ListEmpty(Q1) )
		{
			if( ps >= n-dropped )
			{ 
				stop = 1; 
			}
			pthread_mutex_unlock( &m );
		}
		else
		{
			temp = My402ListFirst(Q1);
			temp1 = temp->obj;
			if( temp1->ntokens <= cur )
			{
				cur-= temp1->ntokens;
				gettimeofday(&tv1, NULL);
				t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
				My402ListAppend(Q2, temp1);
				pthread_cond_signal(&cv);
				My402ListUnlink(Q1, temp);
				//totq1+=(double)t-(temp1->tq1);
				temp1->tq1l = t;
				printf("%012.3fms: %s leaves Q1, time in Q1 = %.3fms, token bucket now has %ld token\n",(double)t,temp1->name, (double)t-(temp1->tq1),cur );
				gettimeofday(&tv1, NULL);
				t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
				temp1->tq2 = t;
				printf("%012.3fms: %s enters Q2\n",(double)t, temp1->name);
				ps++;
				if( ps >= n-dropped )
				{ 
					stop = 1; 
				}
				pthread_mutex_unlock( &m );
			}
			else
			{
				pthread_mutex_unlock( &m );
			}
		}
		if(stop == 1) break;//508683  201509103050307
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_testcancel();
		gettimeofday(&tv1, NULL);
		bk2 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		usleep( (1000000/r)-(bk2-bk1)*1000);
		gettimeofday(&tv1, NULL);
		bk1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
		sprintf(tname, "t%ld", ++pct);
		gettimeofday(&tv1, NULL);
		t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		if( cur == B )
		{
			printf("%012.3fms: Token %s dropped\n",(double)t,tname);
			tdropped++;	
		}
		else if( cur<B )
		{
			cur++;
			printf("%012.3fms: token %s arrives, token bucket now has %ld token\n",(double)t,tname, cur);
		}
	}
	pthread_mutex_lock( &m );
	pthread_cond_broadcast(&cv);
	pthread_mutex_unlock( &m );
	pthread_cleanup_pop(0);
	return 0;
}

void clean_S()
{
	pthread_mutex_unlock( &m );
}
void *S( void *id )
{
	double t,t1,bk1,bk2;
	struct timeval tv1;
	gettimeofday(&tv1, NULL);
	bk1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
	long id1 = (long)id;
	pthread_cleanup_push(clean_S, NULL);
	packet *temp1;
	My402ListElem *temp;
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );
	while(1)
	{
		pthread_mutex_lock( &m );
		while( My402ListEmpty(Q2) && pserv != n-dropped)
		{
			pthread_cond_wait( &cv, &m );
		}
		if( pserv == n-dropped )
		{
			if( id1 == 1 ) pthread_cancel( s2 );
			else pthread_cancel( s1 );
			pthread_mutex_unlock( &m );
			break;
		}
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
		temp = My402ListFirst(Q2);
		temp1 = temp->obj;
		gettimeofday(&tv1, NULL);
		t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		//totq2 += (double)t-(temp1->tq2);
		temp1->tq2l = t;
		printf("%012.3fms: %s leaves Q2, time in Q2 = %.3fms\n", (double)t, temp1->name, (double)(t-(temp1->tq2)));
		My402ListUnlink( Q2, temp );
		printf("%012.3fms: %s begins service at S%ld, requesting %.3fms of service\n",(double)t, temp1->name, id1,temp1->s_time*1000 );
		pthread_mutex_unlock( &m );
		gettimeofday(&tv1, NULL);
		bk2 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		usleep( (temp1->s_time*1000000) );
		gettimeofday(&tv1, NULL);
		t1 = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
		bk1 = t1;
		if( id1 == 1 )
		{
			tots1 += (double)t1-t; 
		}
		else 
			tots2 += (double)t1-t;
		pthread_mutex_lock( &m );
		servt+=(double)t1-t;
		printf("%012.3fms: %s departs from S%ld, service time = %.3fms, time in system = %.3fms\n",(double)t1, temp1->name, id1, (double)t1-t,(double)t1-(temp1->tstrt));
		totavg += (double)t1-(temp1->tstrt);
		totavg2 += (double)(t1-(temp1->tstrt))*(t1-(temp1->tstrt));
		++pserv;
		totq1+=(double)(temp1->tq1l)-(temp1->tq1);
		totq2+=(double)(temp1->tq2l)-(temp1->tq2);
		pthread_mutex_unlock( &m );
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_testcancel();
	}
	pthread_cleanup_pop(0);
	return 0;
}

void *signal_hand()
{
	int c;
	double t;
	struct timeval tv1;
	My402ListElem *temp;
	packet *temp1;
	sigwait( &set, &c );
	pthread_mutex_lock( &m );
	gettimeofday(&tv1, NULL);
	t = (tv1.tv_sec-tv.tv_sec)*1000 + ( tv1.tv_usec-tv.tv_usec )/1000.0;
	while(!My402ListEmpty(Q1))
	{
		temp = My402ListFirst(Q1);
		temp1 = temp->obj;
		My402ListUnlink( Q1, My402ListFirst(Q1) );
		printf("%012.3f: %s removed from Q1\n",(double)t,temp1->name);
	}
	while(!My402ListEmpty(Q2))
	{
		temp = My402ListFirst(Q2);
		temp1 = temp->obj;
		My402ListUnlink( Q2, My402ListFirst(Q2) );
		printf("%012.3f: %s removed from Q2\n",(double)t,temp1->name);
	}
	pthread_mutex_unlock( &m );
	pthread_cancel( tb ); 
	pthread_cancel( pg );
	pthread_cancel( s1 );
	pthread_cancel( s2 );
	return 0;
}

int main(int argc, char **argv)
{
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);
	Q1 = (My402List *)malloc( sizeof(My402List) );
	Q2 = (My402List *)malloc( sizeof(My402List) );
	f = stdin;
	char buff[4];
	static struct option opts[] = {
	{ "lambda", 1, 0 ,0 },
	{ "mu", 1, 0, 0 }
	};
	char c;
	struct stat info;
	printf("Emulation Parameters:\n");
	while(1)
	{
		c = getopt_long_only( argc, argv, "-B:P:n:r:t:",opts, &opt );
		if( c == -1 )
			break;
		switch(c)
		{
			case 0 : if ( opt==1 )
				   {
				   	mu = atof(optarg); 
				   }
				   else if ( opt==0 )
				   {
				   	lambda = atof(optarg); 
				   }
				   break;
			case 'B' : B = atof(optarg);
				   break;
			case 'P' : P = atoi(optarg);
				   
				   break;
			case 'n' : n = atoi(optarg);
				   
				   break;
			case 'r' : r = atof(optarg);
				   if( 1/r > 10.00)
				   	r = 1/10.00;
				   
				   break;
			case 't' : stat(optarg, &info);
				   if( S_ISDIR(info.st_mode) )
				   {
					printf("Input file is a directory\nExiting Program\n");
					return 0;
				   }
				   f = fopen(optarg, "r");
				   if ( f == NULL )
				   {
				   	perror("Couldn't open file: ");
				 	return 0;
				   }
				   printf("\ttsfile = %s\n",optarg);
				   fgets( buff, sizeof(buff), f );
				   n = atoi(buff);
				   break;
		}
		
	}
	if( f == stdin )
	{
		printf( "\tlambda = %f\n", lambda );
		printf( "\tmu = %f\n", mu );
		printf( "\tP = %ld\n", P );
		printf( "\tnumber to arrive = %ld\n", n );
	}
	printf( "\tB = %f\n", B);
	printf( "\tr = %f\n\n", r );
	
	gettimeofday(&tv, NULL);
	printf("00000000.000ms: emulation begins\n");
	pthread_create( &pg, NULL, token_bucket_filter, NULL);
	pthread_create( &tb, NULL, Packet_Gen, NULL);
	pthread_create( &s1, NULL, S, (void*)1);
	pthread_create( &s2, NULL, S, (void*)2);
	pthread_create( &sig, NULL, signal_hand, NULL);
	pthread_join( pg, 0);
	pthread_join( tb, 0);
	pthread_join( s1, 0);
	pthread_join( s2, 0);
	pthread_cancel( sig );
	pthread_join( sig, 0);
	finish();
	return 0;
}
