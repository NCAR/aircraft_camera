#include "multicast.h" 

#define MULTICAST_PORT 30001
#define MULTICAST_GROUP "239.0.0.10"

void multicast_status_init(status_t *status, camConf_t **camArray, int numCams) {
/* This function allocates memory for all of the data contained in the status 
	structure. The size of memory needed varies by the number of cameras on 
	the bus as well as the current settings in the config file.
*/
	int i;
	strcpy(status->clock, "0000-00-00 00:00:00");
	status->health = 0;
	status->html = 0;
	status->numCams = numCams;
//	printf("numcams: %d", numCams);
	
	status->lastSize = malloc(sizeof(int) * numCams);
	status->runningAvg = malloc(sizeof(float) * numCams);
	status->cam_row = malloc(sizeof(char *) * numCams);
	status->latest = malloc(sizeof(char *) * numCams);

	for	(i=0; i<numCams; i++) {
		status->cam_row[i] = malloc(sizeof(char) * (
			strlen(ROW_BASE) + strlen(camArray[i]->direction) + 16
		));
		sprintf(status->cam_row[i] ,ROW_BASE, camArray[i]->direction, camArray[i]->guid);

		status->latest[i] = malloc(sizeof(char) * 100);
		status->lastSize[i] = 0;
		status->runningAvg[i] = 0;
	}

//	printf("%s\n", status->cam_row[0]);

	return;
}

void multicast_send_final(status_t *status) {
/* This function sends the final status multicast packet before the program terminates.
	It sets health to 0, because the program is ending. Then it sends a packet with
	'shut down' as the message. 
*/
	char *pkt;
	pkt = malloc(sizeof(char) * (strlen(PACKET_BASE) + strlen(status->clock) + 11));
	sprintf(pkt, PACKET_BASE, status->clock, 0, "shut down"); 
	multicast_send_packet(pkt, MULTICAST_GROUP, MULTICAST_PORT);
	free(pkt);
}

void multicast_clean_up(status_t *status){
/* This function sends a final multicast message, then cleans up memory that
	was allocated by multicast_status_init()
*/
	int i;

	status->health = 0;	
	multicast_send_final(status);

	for (i=0; i<status->numCams; i++){
		free(status->cam_row[i]);
		free(status->latest[i]);
	}
	free(status->cam_row);
	free(status->latest);
	free(status->lastSize);
	free(status->runningAvg);

	return;
}

int multicast_send_status(status_t *status) {
/* This function takes the data from a status_t struct and arranges it into the 
	standard multicast packet form described here: http://wiki.eol.ucar.edu/sew/Aircraft/Handbook
*/	
	char *rows, *packet;
	char row[500];
	int i;

	/* calculate each cameras new running avg (poor mans averaging) */
	for (i=0; i<status->numCams; i++) {
		status->runningAvg[i] = ( 8 * status->runningAvg[i] + status->lastSize[i]) / 9.0;
	}

	/* allocate string to hold a (html table) row for each cam */
	rows = malloc( sizeof(char) * 500 * status->numCams);
	if (!rows) return 0; //could not allocate space for rows!
	rows[0] = '\0';

	/* write data specific to each cam into a row, add to to "rows" string */
	for (i=0; i<status->numCams; i++) {
		sprintf(row, "<tr>%s<td>%s</td><td>%d B</td><td>%f B</td></tr>", status->cam_row[i], 
			status->latest[i], status->lastSize[i], status->runningAvg[i]);
		strcat(rows, row);
	}

	/* make sure "html" string has enough space for all the data */ 
	status->html = (char*) realloc(status->html, sizeof(char) * 
		(strlen(TABLE_BASE) + strlen(rows)));
	if (!status->html) { //could not allocate space for html!
		free(rows);
		return 0; 
	}

	/* build final "html" string */
	sprintf(status->html, TABLE_BASE, rows);

	/* make sure "packet" string has enough space for all the data */ 
	packet = malloc( sizeof(char) * 
		(strlen(PACKET_BASE) + strlen(status->clock) + strlen(status->html)));
	if (!packet) { //could not allocate space for html!
		free(rows);
		return 0; 
	}

	/* build final packet to be sent via multicast */
	sprintf(packet, PACKET_BASE, status->clock, status->health, status->html);

	/* open socket and send the packet */
	multicast_send_packet(packet, MULTICAST_GROUP, MULTICAST_PORT);

	/* clean up memory */
	free(rows);
	free(packet);
	return 1;
}

void multicast_send_packet(char *packet, char *group, int port) {

	struct sockaddr_in addr;
	int fd;
	
	/* create what looks like an ordinary UDP socket */
	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		syslog(LOG_ERR, "failed to create Multicast socket");
		return;
	}
	
	/* set up destination address */
	memset(&addr,0,sizeof(addr)); 	//zero out addr struct
	addr.sin_family=AF_INET;		//set to inet socket (i.e. not UNIX)
	addr.sin_addr.s_addr=inet_addr(group);	//set IP address (multicast group)
	addr.sin_port=htons(port);		//set port to send data on
	
	/* now just sendto() our destination! */
	if (sendto(fd,packet,strlen(packet),0,(struct sockaddr *) &addr,
	sizeof(addr)) < 0) {
		syslog(LOG_ERR, "failed to send Multicast status");
		return;
	}

	/* close the socket - important */
	close(fd);

	return;
}
