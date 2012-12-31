void *get_in_addr(struct sockaddr *sa);
void handle_connection(int sock, struct sockaddr_storage their_addr);
int prepare_response(char resource[], char **xml, char *query);
int send_file(int sock, char *path);
