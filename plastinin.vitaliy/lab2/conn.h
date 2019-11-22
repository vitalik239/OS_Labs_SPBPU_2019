#pragma once

#include <cstdlib>

class Conn
{
public:
	Conn(size_t id, bool create);
	bool Read(void *buf, size_t count);
	bool Write(void *buf, size_t count);
	~Conn();
};
