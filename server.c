#include "server.h"
#include "anet.h"
#include "log.h"
#include "protocol.h"


int dictLookupKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
	lookupKey *lk1 = (lookupKey *)key1;
	lookupKey *lk2 = (lookupKey *)key2;
    int l1,l2;
    if (lk1->type != lk2->type) {
    	return 0;
    }
    l1 = strlen(lk1->name);
    l2 = strlen(lk2->name);
    if (l1 != l2) return 0;
    return memcmp(lk1->name, lk2->name, l1) == 0;
}

void dictLookupKeyDestructor(void *privdata, void *key)
{

	zfree(((lookupKey *)key)->name);
	zfree(key);
}

void dictLookupValueDestructor(void *privdata, void *obj)
{
	zfree(obj);
}

unsigned int dictLookupHashFunction(const void *key)
{
	lookupKey *lk = (lookupKey*)key;
	unsigned int ret = dictGenHashFunction(lk->name, strlen(lk->name));
	return ret ^ (lk->type);
}

dictType stringDictType = {
    dictLookupHashFunction,       /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictLookupKeyCompare,         /* key compare */
    dictLookupKeyDestructor, 	   /* key destructor */
    dictLookupValueDestructor     /* val destructor */
};

char* findLookupVal(char *key, unsigned char type)
{
	lookup_log(LOG_DEBUG, "table find %s,%d\n", key, type);
	lookupKey find;
	find.name = key;
	find.type = type;
	char *val = (char *)dictFetchValue(dt, &find);
	return val;
}

int addLookupEntry(char *key, char *value, unsigned char type)
{
	lookupKey *lookKey = zmalloc(sizeof(lookupKey));
	lookKey->name = strdup(key);
	lookKey->type = type;
	lookup_log(LOG_DEBUG, "table add %s,%d\n", key, type);
	dictAdd(dt, (void *)lookKey, (void *)value);
	return 0;
}

int initLookupTable(void)
{
	dt = dictCreate(&stringDictType, NULL);
	return 0;
}

int serverWrite(event *ev, int fd) {
	lookup_log(LOG_DEBUG, "write\n");
	int write = 0;
	if (ev->wr_pos == 0) {
		ev->body_data = build_request_str(ev->data);
	}
	write = anetWrite(fd, ev->body_data, ev->data->body_len  + PROTOCOL_HAEDER_SIZE);
	if (write == 0 && errno == ECONNRESET) {
		lookup_log(LOG_DEBUG, "client close conn\n");
	}
	ev->wr_pos == 0;
	if (del_event(pool, fd) < 0) {
		printf("%s\n", strerror(errno));
		exit(0);
	}

	// dump_request(ev->data);

	int tmp = 0;
	int i = 0;
	while (tmp < ev->data->body_len) {
		lookup_log(LOG_DEBUG, "free:%s\n", ev->data->body[i].data);
		zfree(ev->data->body[i].data);
		tmp += ev->data->body[i].len + 3;
		zfree(ev->data->body[i]);
		i++;
	}	
	close(fd);
	zfree(ev->data);
	zfree(ev->body_data);
	lookup_log(LOG_DEBUG, "close fd\n");
	return 0;
}

int serverRead(event *ev, int fd) {
	lookup_log(LOG_DEBUG, "read\n");
	int read = 0;
	char buf[IO_PACKET_SIZE];

	if (ev->wr_pos == 0) {
		read = anetRead(fd, &buf[0], PROTOCOL_HAEDER_SIZE);
		if (read == 0 && errno == ECONNRESET) {
free_conn:
			ev->wr_pos = 0;
			del_event(pool, fd);	
			close(fd);
			zfree(ev->data);
			lookup_log(LOG_DEBUG, "close fd\n");
			return 0;
		}
		ev->data = parse_protocol_header(buf);
	} else {
		int block = (ev->data->body_len + PROTOCOL_HAEDER_SIZE - ev->wr_pos) 
			/ IO_PACKET_SIZE;
		if (block > 0) {
			read = anetRead(fd, &buf[0], IO_PACKET_SIZE);			
		} else {
			read = anetRead(fd, &buf[0], 
				(ev->data->body_len + PROTOCOL_HAEDER_SIZE - ev->wr_pos) % IO_PACKET_SIZE);			
		}
		if (read == 0 && errno == ECONNRESET) {
			goto free_conn;
		}
		if (ev->body_data == NULL) {
			ev->body_data_size = IO_PACKET_SIZE * 2;
			ev->body_data = (char *)zmalloc(ev->body_data_size);
			memcpy(ev->body_data + ev->body_data_wr_pos, buf, read);
			ev->body_data_wr_pos += read;
		} else {
			if ((ev->body_data_wr_pos + read) > ev->body_data_size) {
				//resize
				ev->body_data_size = ev->body_data_size * 2;
				ev->body_data = (char *)zrealloc(ev->body_data, 
					ev->body_data_size);
			}
			memcpy(ev->body_data + ev->body_data_wr_pos, buf, read);
			ev->body_data_wr_pos += read;
		}
	}
	if (read > 0) {
		ev->wr_pos += read;	
	}
	lookup_log(LOG_DEBUG, "pos:%d\n", ev->wr_pos);
	if (ev->wr_pos >= (ev->data->body_len +  PROTOCOL_HAEDER_SIZE)) {
		parse_protocol_body(ev->body_data, ev->data);
		ev->wr_pos = 0;
		processRequest((lookup_protocol *)ev->data);
		ev->cb = serverWrite;
		set_event(pool, fd, EPOLLOUT);
	}
	return 0;
}

int processRequest(lookup_protocol *pro)
{
	int nread = 0;
	int nwrite = 0;
	int body_count = 0;
	int old_len = 0;
	while (nread < pro->body_len) {
		lookup_body *b = &pro->body[body_count];
		char *val = findLookupVal(b->data, b->type);
		old_len += b->len;
		b->len = strlen(val) + 1;
		b->data = zrealloc(b->data, b->len);
		memcpy(b->data, val, b->len);
		nread += old_len + 3;
		nwrite += b->len + 3;
		body_count++;
	}
	pro->body_len = nwrite;
	SET_ANSWER(pro->info);
	return 0;
}

void initServer(char *host, int port) {
	char *msg;
	int listendfd = anetTcpServer(msg, port, host);
	if (listendfd <= 0) {
		lookup_log(LOG_DEBUG, "create server error %s\n", msg);
		exit(0);
	}
	lookup_log(LOG_DEBUG, "listened %s:%d\n", host, port);
	if (anetNonBlock(msg, listendfd) < 0) {
		lookup_log(LOG_DEBUG, "set noblocking server error %s\n", msg);
		exit(0);		
	}
	pool = create_event(1024);
	add_event(pool, listendfd, EPOLLIN, serverAccept);
	while (1) {
		lookup_log(LOG_DEBUG, "polling\n");
		int num = handle_event(pool, 30);
		int i;
		for (i=0; i<num; i++) {
			int fd = pool->epollEv[i].data.fd;
			noti_chain_callback cb = pool->events[fd].cb;
			(*cb) (&pool->events[fd], fd);
		}
	}
	close(listendfd);
}

int serverAccept(event *v, int fd) {
	char *msg;
	char clientHost[32];
	int clientPort, clientFd;
	if ((clientFd = anetTcpAccept(msg, fd, &clientHost[0], &clientPort)) > 0) {
		lookup_log(LOG_DEBUG, "accept %s:%d fd:%d\n", clientHost, clientPort, clientFd);
		if (anetNonBlock(msg, clientFd) < 0) {
			return -1;
		}	
		add_event(pool, clientFd, EPOLLIN, serverRead);
	}
	return 0;
}