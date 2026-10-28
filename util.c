#include "util.h"

int dnslookup(const char* hostname, char* firstIPstr, int maxSize){

    //local vars
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct in_addr* ipv4addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int addrError = 0;

    //debug
#ifdef UTIL_DEBUG
    fprintf(stderr, "%s\n", hostname);
#endif
   
    //lookup hostname
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
	fprintf(stderr, "Error looking up Address: %s\n",
		gai_strerror(addrError));
	return UTIL_FAILURE;
    }
    //loop through linked list result
    for(result=headresult; result != NULL; result = result->ai_next){
	//extract ip address, convert to string
	if(result->ai_addr->sa_family == AF_INET){
	    //IPv4 address
	    ipv4sock = (struct sockaddr_in*)(result->ai_addr);
	    ipv4addr = &(ipv4sock->sin_addr);
	    if(!inet_ntop(result->ai_family, ipv4addr,
			  ipv4str, sizeof(ipv4str))){
		perror("Error Converting IP to String");
		return UTIL_FAILURE;
	    }
#ifdef UTIL_DEBUG
	    fprintf(stdout, "%s\n", ipv4str);
#endif
	    strncpy(ipstr, ipv4str, sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	else if(result->ai_addr->sa_family == AF_INET6){
	    //IPv6 address
#ifdef UTIL_DEBUG
	    fprintf(stdout, "IPv6 Address: Not Handled\n");
#endif
	    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	else{
	    //unhandlded protocol
#ifdef UTIL_DEBUG
	    fprintf(stdout, "Unknown Protocol: Not Handled\n");
#endif
	    strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
	    ipstr[sizeof(ipstr)-1] = '\0';
	}
	//save first ip address
	if(result==headresult){
	    strncpy(firstIPstr, ipstr, maxSize);
	    firstIPstr[maxSize-1] = '\0';
	}
    }

    //cleanup
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}
