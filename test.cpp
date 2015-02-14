#include<iostream>
using namespace std;

template <class T>
struct ST
{
	T n;
	int count;
};

template <class T>
class A
{
	T m;
	static struct ST<T> st;
	public:
		A(T a):m(a){st.n+=m; st.count++; cout<<"construct"<<endl;}
		void disp(){cout<<"m="<<m<<", n="<<st.n<<", cnt="<<st.count<<endl;}
};

template <class T>
struct ST<T> A<T>:: st = {
	.n = 0,
	.count = 0
}; // 静态数据成员的初始化


int main()
{
	A<int> a(2), b(3);
	a.disp();
	b.disp();
	A<double> c(1.2),d(4.6);
	c.disp();
	d.disp();
	return 0;
}

