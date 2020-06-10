/*
 * gxplatform.h
 *
 *  Created on: 4 Jun 2020
 *      Author: gwinilts
 */

#ifndef GXPLATFORM_H_
#define GXPLATFORM_H_


#ifdef __ANDROID__




#include <android/log.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <sstream>

#include <jni.h>



//#define logx(...) __android_log_print(ANDROID_LOG_INFO, "Cpp.out", __VA_ARGS__)

inline void logx() {}

template<typename First, typename ...Rest>
inline void logx(First && first, Rest && ...rest)
{
	std::stringstream ___logstream;

	___logstream.str("");

	___logstream << first;
	__android_log_print(ANDROID_LOG_INFO, "Cpp.out", ___logstream.str().c_str(), NULL);
	logx(std::forward<Rest>(rest)...);
}

typedef AAsset G_FILE;

#define G_RSC_PREFIX "rsc/"

#define g_fread(buf, bsize, count, fd) AAsset_read(fd, buf, count)

inline bool g_file_exists(void* ast, std::string name, const char* prefix = "") {
	AAssetManager *astm = (AAssetManager *)ast;

	const char* found_name;

	AAssetDir *asd = AAssetManager_openDir(astm, prefix);
	AAsset *as;

	do {
		found_name = AAssetDir_getNextFileName(asd);

		if (name.compare(found_name) == 0) {
			AAssetDir_close(asd);
			return true;
		}
	} while (found_name != NULL);

	AAssetDir_close(asd);
	return false;
}

inline G_FILE* g_file_open(void* ast, std::string name, const char* prefix = "") {
	AAssetManager *astm = (AAssetManager *)ast;

	std::string path = std::string(prefix) + name;

	return AAssetManager_open(astm, path.c_str(), AASSET_MODE_BUFFER);
}

inline int g_total_read(G_FILE *fd, char * &buf) {
	size_t size = AAsset_getLength(fd);
	buf = new char[size];

	AAsset_read(fd, buf, size);

	return size;
}

inline void g_file_close(G_FILE *fd) {
	AAsset_close(fd);
}






#else

#include <string>
#include <fstream>
#include <iostream>

#define g_fread(buf, bsize, count, fd) fread(buf, bsize, count, fd)

#define G_RSC_PREFIX "/srv/http/wrapp/rsc/"

#include <utility>

inline void logx(){}

template<typename First, typename ...Rest>
inline void logx(First && first, Rest && ...rest)
{
    std::cout << std::forward<First>(first) << std::endl;
    logx(std::forward<Rest>(rest)...);
}

typedef FILE G_FILE;

inline bool g_file_exists(void* ast, std::string filename, const char* prefix = "./") {
	std::string path = std::string(prefix) + filename;

	std::ifstream infile(path.c_str());

	return infile.good();
}

inline G_FILE *g_file_open(void* ast, std::string name, const char* prefix = "/srv/http/wrapp/") {
	std::string path = std::string(prefix) + name;

	return fopen(path.c_str(), "r");
}

inline void g_file_close(G_FILE * file) {
	fclose(file);
}

inline int g_total_read(G_FILE * fd, char * &buf) {
	char _buf[2048];
	size_t size = 0;
	size_t read;
	char* tmp;

	buf = new char[0];

	do {
		read = fread(_buf, sizeof(char), 2048, fd);

		if (read > 0) {
			tmp = buf;

			buf = new char[size + read];

			for (size_t i = 0; i < size; i++) {
				buf[i] = tmp[i];
			}

			delete [] tmp;

			for (size_t i = 0; i < read; i++) {
				buf[i + size] = _buf[i];
			}

			size += read;
		}
	} while (read > 0);

	return size;
}

#endif


#endif /* GXPLATFORM_H_ */
