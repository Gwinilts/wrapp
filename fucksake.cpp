//============================================================================
// Name        : fucksake.cpp
// Author      : gwinilts
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <string>

#include "./HTTP.h"
#include "./ClientThread.h"
#include "./mqueue/mqueue.h"
#include "./network/networklayer.h"
#include "./binman/binman.h"
#include "./chess/chess.h"

#include "./gxplatform.h"

#define PORT 8080

using namespace std;

int server_start(sockaddr_in &addr, int addrlen) {
	int sock, opt = 1;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		cout << "error creating socket." << endl;
		logx("error couldn't create socket");
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // should we really do this? what if another isnstance is still running?
		cout << "could not set options" << endl;
		logx("could not set options");
		return -1;
	}

	if (::bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		logx("could not bind socket");
		return -1;
	}

	logx("cannot check if bind address actually worked...");

	/*
	if (< 0) {
		cout << "error on bind" << endl;
		return -1;
	}*/

	if (listen(sock, 5) < 0) {
		cout << "error on listen" << endl;
		logx("could not listen socket");
		return -1;
	}

	return sock;
}


int init(void *ast, string bcast) {
	struct sockaddr_in addr;

	int sock, client, addrlen = sizeof(addr);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	logx("will call server_Start");

	sock = server_start(addr, addrlen);

	ClientThread *c;

	binman *bin = new binman;
	bin->start();

	logx("threadloop will bgin");

	while (true) {

		logx("pending accept");

		client = accept(sock, (struct sockaddr *) &addr, (socklen_t*) &addrlen);

		if (client < 0) {
			logx("bad accept");
		} else {
			logx("accept ok.");
		}

		c = new ClientThread(client, ast, bcast, bin);
		c->start();
	}
	return 0;
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}


#ifdef __ANDROID__

extern "C" JNIEXPORT jint JNICALL
Java_com_gwinilts_cpptestagain_NativeThread_init(JNIEnv* env, jobject obj, jobject assetManager, jbyteArray bcast) {
	logx("startup is on an android device, yay!");
	AAssetManager *ast = AAssetManager_fromJava(env, assetManager);

	unsigned char b = 1;

	jbyte *bf = env->GetByteArrayElements(bcast, &b);
	jsize len = env->GetArrayLength(bcast);

	char* _buf = new char[len];

	for (jsize i = 0; i < len; i++) {
		_buf[i] = bf[i];
	}

	logx(_buf);

	env->ReleaseByteArrayElements(bcast, bf, 0);

	return init((void*)ast, std::string(_buf, len));
}

#endif


int main(int argc, char **argv) {
	void *fake = NULL;

	const char * cmd = "ip -j -p -f inet address show wlo1 | grep broadcast | grep -o -E '([0-9]{1,3}[.]{0,1})+'";
	string bcast = exec(cmd);

	logx("startup is on a linux machine, yay!");

	return init(fake, string(bcast.c_str(), bcast.length() - 1));
}
