/*
   3APA3A simpliest proxy server
   (c) 2002-2008 by ZARAZA <3APA3A@security.nnov.ru>

   please read License Agreement

   $Id: smtpp.c,v 1.13 2011-06-10 20:48:46 vlad Exp $
*/

#include "proxy.h"

#define RETURN(xxx) { param->res = xxx; goto CLEANRET; }

char  ehlo[] = 	"250-Proxy\r\n"
		"250-AUTH PLAIN LOGIN\r\n"
		"250-8BITMIME\r\n"
		"250 DSN\r\n";

int readreply (struct clientparam* param) {
 unsigned char * buf;
 int res, i, bufsize = 640;

 if(!(buf = myalloc(bufsize))) return 0;
 do {
	i = sockgetlinebuf(param, SERVER, buf, bufsize-1, '\n', conf.timeouts[STRING_L]);
	if(i < 1) break;
#ifndef WITHMAIN
	res = handlehdrfilterssrv(param, &buf, &bufsize, 0, &i);
	if(res != PASS) {
		myfree(buf);
		return -1;
	}
#endif
	socksend(param->clisock, buf, i, conf.timeouts[STRING_S]);	
 } while (i > 3 && buf[3] == '-');
 if(i < 3) {
	myfree(buf);
	return 0;
 }
 buf[i] = 0;
 res = atoi(buf);
 myfree(buf);
 return res;
}

int readcommand (struct clientparam* param) {
 unsigned char * buf;
 int res, i, bufsize = 320;
 int ret = 1;

 if(!(buf = myalloc(bufsize))) return 0;
 i = sockgetlinebuf(param, CLIENT, buf, bufsize-1, '\n', conf.timeouts[STRING_L]);
 if(i < 4) return 0;
#ifndef WITHMAIN
 if(!strncasecmp(buf, "MAIL", 4) || !strncasecmp(buf, "RCPT", 4) || !strncasecmp(buf, "STARTTLS", 8) || !strncasecmp(buf, "TURN", 4)){
	res = handlehdrfilterscli(param, &buf, &bufsize, 0, &i);
	if(res != PASS) {
		myfree(buf);
		if(res == HANDLED) return 2;
		return -1;
	}
 }
#endif
 socksend(param->remsock, buf, i, conf.timeouts[STRING_S]);	
 myfree(buf);
 if(!strncasecmp(buf, "STARTTLS", 8) || !strncasecmp(buf, "TURN", 4)){
	ret = 22;
 }
 return ret;
}

int readdata (struct clientparam* param) {
 unsigned char * buf;
 int res, i, bufsize = 4096;

 if(!(buf = myalloc(bufsize))) return 0;
 while ((i = sockgetlinebuf(param, CLIENT, buf, bufsize-1, '\n', conf.timeouts[STRING_L])) > 0 && !(i==3 && buf[0] == '.')){
#ifndef WITHMAIN
	res = handledatfltcli(param, &buf, &bufsize, 0, &i);
	if(res != PASS) {
		myfree(buf);
		if(res == HANDLED) return 1;
		return -1;
	}
#endif
	socksendto(param->remsock, (struct sockaddr *)&param->sins, buf, i, conf.timeouts[STRING_S]);	
 } 
 if(i < 1) {
	myfree(buf);
	return 0;
 }
 socksend(param->remsock, buf, i, conf.timeouts[STRING_S]);	
 myfree(buf);
 return 1;
}


void * smtppchild(struct clientparam* param) {
 int i=0, res;
 unsigned char buf[320];
 unsigned char username[256];
 unsigned long ul;
 char * command = NULL;
 int login = 0;

 if(socksend(param->clisock, (unsigned char *)"220 Proxy\r\n", 11, conf.timeouts[STRING_S])!=11) {RETURN (611);}
 i = sockgetlinebuf(param, CLIENT, buf, sizeof(buf) - 10, '\n', conf.timeouts[STRING_S]);
 while(i > 4 && (strncasecmp((char *)buf, "AUTH PLAIN", 10) || !(login = 2)) && (strncasecmp((char *)buf, "AUTH LOGIN", 10) || !(login = 1))){
	if(!strncasecmp((char *)buf, "QUIT", 4)){
		socksend(param->clisock, (unsigned char *)"221 Proxy\r\n", 11,conf.timeouts[STRING_S]);	
		RETURN(0);
	}
	else if(!strncasecmp((char *)buf, "HELO ", 5)){
		socksend(param->clisock, (unsigned char *)"250 Proxy\r\n", 11,conf.timeouts[STRING_S]);	
	}
	else if(!strncasecmp((char *)buf, "EHLO ", 5)){
		socksend(param->clisock, (unsigned char *)ehlo, sizeof(ehlo) - 1,conf.timeouts[STRING_S]);	
	}
	else if(!param->hostname) socksend(param->clisock, (unsigned char *)"571 need AUTH first\r\n", 22, conf.timeouts[STRING_S]);	
	else {
		login = -1;
		buf[i] = 0;
		command = mystrdup(buf);
		break;
	}
	i = sockgetlinebuf(param, CLIENT, buf, sizeof(buf) - 10, '\n', conf.timeouts[STRING_S]);
 }
 if(!login) {RETURN(662);}
 if(login == 1){
	socksend(param->clisock, "334 VXNlcm5hbWU6\r\n", 18,conf.timeouts[STRING_S]);	
	i = sockgetlinebuf(param, CLIENT, buf, sizeof(buf) - 10, '\n', conf.timeouts[STRING_S]);
	if(i < 3) {RETURN(663);}
	buf[i-2] = 0;
	i = de64(buf,username,255);
	if(i < 1) {RETURN(664);}
	username[i] = 0;
	parseconnusername(username, param, 0, 25);
	socksend(param->clisock, "334 UGFzc3dvcmQ6\r\n", 18,conf.timeouts[STRING_S]);	
	i = sockgetlinebuf(param, CLIENT, buf, sizeof(buf) - 10, '\n', conf.timeouts[STRING_S]);
	if(i < 2) {RETURN(665);}
	buf[i-2] = 0;
	i = de64(buf,username,255);
	if(i < 0) {RETURN(666);}
	username[i] = 0;
	if(param->extpassword) myfree(param->extpassword);
	param->extpassword = mystrdup(username);
 }
 else if(login == 2){
	if(i > 13) {
		buf[i-2] = 0;
		i = de64(buf+11,username,255);
	}
	else {
		socksend(param->clisock, "334\r\n", 5,conf.timeouts[STRING_S]);	
		i = sockgetlinebuf(param, CLIENT, buf, sizeof(buf) - 10, '\n', conf.timeouts[STRING_S]);
		if(i < 3) {RETURN(667);}
		buf[i-2] = 0;
		i = de64(buf,username,255);
	}
	if(i < 3 || *username) {RETURN(668);}
	username[i] = 0;
	parseconnusername(username+1, param, 0, 25);
	res = (int)strlen(username+1) + 2;
	if(res < i){
		if(param->extpassword) myfree(param->extpassword);
		param->extpassword = mystrdup(username + res);
	}
 }

#ifndef WITHMAIN
 {
	int action, reqbufsize, reqsize;
	reqbufsize = reqsize = (int)strlen(param->hostname) + 1;
	action = handlereqfilters(param, &param->hostname, &reqbufsize, 0, &reqsize);
	if(action == HANDLED){
		RETURN(0);
	}
	if(action != PASS) RETURN(617);
 }
#endif

 param->operation = CONNECT;
 res = (*param->srv->authfunc)(param);
 if(res) {RETURN(res);}
 do {
	i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
 } while (i > 3 && buf[3] == '-');
 if( i < 3 ) {RETURN(671);}
 buf[i] = 0;
 if(strncasecmp((char *)buf, "220", 3)||!strncasecmp((char *)buf+4, "PROXY", 5)){RETURN(672);}
 ul = param->extip;
 i = sprintf(buf, "EHLO [%lu.%lu.%lu.%lu]\r\n", ((ul&0xFF000000)>>24), ((ul&0x00FF0000)>>16), ((ul&0x0000FF00)>>8), ((ul&0x000000FF)));
 if(socksend(param->remsock, buf, i, conf.timeouts[STRING_S])!= i) {RETURN(673);}
 param->statscli64+=i;
 param->nwrites++;
 login = 0;
 do {
	i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
	if(i < 1) break;
	buf[i] = 0;
	if(strstr((char *)buf, "LOGIN")) login |= 1;
	if(strstr((char *)buf, "PLAIN")) login |= 2;

 } while (i > 3 && buf[3] == '-');
 if(i<3) {RETURN(672);}
 if(!command || (param->extusername && param->extpassword)){
	if(!param->extusername || !*param->extusername || !param->extpassword || !*param->extpassword || !login){
		socksend(param->clisock, "235 auth required\r\n", 19,conf.timeouts[STRING_S]);	
	}
	if ((login & 1)) {
		socksend(param->remsock, "AUTH LOGIN\r\n", 12, conf.timeouts[STRING_S]);
 param->statscli64+=12;
		param->nwrites++;
		i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
		if(i<4 || strncasecmp((char *)buf, "334", 3)) {RETURN(680);}
		en64(param->extusername, buf, (int)strlen(param->extusername));
		socksend(param->remsock, buf, (int)strlen(buf), conf.timeouts[STRING_S]);
		socksend(param->remsock, "\r\n", 2, conf.timeouts[STRING_S]);
 param->statscli64+=(i+2);
		param->nwrites+=2;
		i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
		if(i<4 || strncasecmp((char *)buf, "334", 3)) {RETURN(681);}
		en64(param->extpassword, buf, (int)strlen(param->extpassword));
		socksend(param->remsock, buf, (int)strlen(buf), conf.timeouts[STRING_S]);
		socksend(param->remsock, "\r\n", 2, conf.timeouts[STRING_S]);
 param->statscli64+=(i+2);
		param->nwrites+=2;
	}
	else if((login & 2)){
		socksend(param->remsock, "AUTH PLAIN\r\n", 12, conf.timeouts[STRING_S]);
 param->statscli64+=(12);
		param->nwrites++;
		i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
		if(i<4 || strncasecmp((char *)buf, "334", 3)) {RETURN(682);}
		*username = 0;
		i = (int)strlen(param->extusername) + 1;
		memcpy(username+1, param->extusername,  i);
		i++;
		res = (int)strlen(param->extpassword);
		memcpy(username + i, param->extpassword, res);
		i+=res;
		en64(username, buf, i);
		i = (int)strlen(buf);
		socksend(param->remsock, buf, i, conf.timeouts[STRING_S]);
		socksend(param->remsock, "\r\n", 2, conf.timeouts[STRING_S]);
 param->statscli64+=(i+2);
		param->nwrites+=2;
	}
	if(command) {
		i = sockgetlinebuf(param, SERVER, buf, sizeof(buf) - 1, '\n', conf.timeouts[STRING_L]);
	}
	res = 1;
 }
 if(command) {
	res = 1;
#ifndef WITHMAIN
	if(!strncasecmp(command, "MAIL", 4) || !strncasecmp(command, "RCPT", 4) || !strncasecmp(command, "STARTTLS", 8) || !strncasecmp(command, "TURN", 4)){
		res = (int)strlen(command);
		command[res] = 0;
		res = handlehdrfilterscli(param, &command, &res, 0, &res);
		if(res != PASS) {
			if(res == HANDLED) res = 2;
			else RETURN(677);
		}
		else if(!strncasecmp(command, "STARTTLS", 8) || !strncasecmp(command, "TURN", 4))
			res = 22;
	}
#endif
	i = (int)strlen(command);
	if(res != 2) socksend(param->remsock, command, i, conf.timeouts[STRING_S]);
 }
#ifndef WITHMAIN

 if(param->nhdrfilterscli || param->nhdrfilterssrv || param->ndatfilterscli || param->ndatfilterssrv){
	do {
		if(res == 22) RETURN (sockmap(param, 180));
		if(res != 2 && (res = readreply(param)) <= 0) break;
		if(res == 221) RETURN(0);
		if(res == 354) res = readdata(param);
		else res = readcommand(param);
	} while(res > 0);
	if(res == 22) 
	if(res >= 0) RETURN(0);
	RETURN(676);
 }
 else

#endif
 
 	RETURN (sockmap(param, 180));

CLEANRET:

 if(param->hostname&&param->extusername) {
	sprintf((char *)buf, "%.64s@%.128s%c%hu", param->extusername, param->hostname, (ntohs(param->sins.sin_port)==25)?0:':', ntohs(param->sins.sin_port));
	 (*param->srv->logfunc)(param, buf);
 }
 else (*param->srv->logfunc)(param, NULL);
 if(param->clisock != INVALID_SOCKET) {
	if ((param->res > 0 && param->res < 100) || (param->res > 661 && param->res <700)) socksend(param->clisock, (unsigned char *)"571 \r\n", 6,conf.timeouts[STRING_S]);
 }
 if(command) myfree(command);
 freeparam(param);
 return (NULL);
}

#ifdef WITHMAIN
struct proxydef childdef = {
	smtppchild,
	25,
	0,
	S_SMTPP,
	" -hdefault_host[:port] - use this host and port as default if no host specified\n"

};
#include "proxymain.c"
#endif
