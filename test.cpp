#include "EventSystem.h"
#include <iostream>

void foo(int i, int& j)
{
	std::cout << i << std::endl;
	j = i;
	i++;
}

struct Functor
{
	void foo(const int& i)
	{
		std::cout << i << std::endl;
	}
	void operator()(const std::function<void()>& f)
	{
		f();
	}
};

int main()
{
	EventSystem es;

	es.listen("foo", foo);
	int i = 2;
	int j = 4;
	es.send("foo", i, j);
	std::cout << i << ' ' << j << std::endl;

	es.listen("foo2", [](int i, int& j)
	{
		std::cout << i << std::endl;
		j = i+1;
		i++;
	});
	es.send("foo2", 2, j);
	std::cout << i << ' ' << j << std::endl;

	Functor f;
	es.listen("foo3", &f, &Functor::foo);
	es.send("foo3", 5);

	es.listen("foo4", f);
	es.send("foo4", std::function([]() {std::cout << "lambda!" << std::endl; }));
}