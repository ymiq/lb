#include <iostream>
#include <tr1/memory>

using namespace std;

class A {
public:
	A() {
		cout << "construct A!!!" << endl;
	}
	;
	~A() {
		cout << "destruct A!!!" << endl;
	}
	;
};


class B: public A {
public:
	B() {
		cout << "construct B!!!" << endl;
	}
	;
	~B() {
		cout << "destruct B!!!" << endl;
	};
	void dump(void) {
		cout << "dump B!!!" << endl;
	};
};


int sptr_main (int argc, char *argv[]) {
	B* ptrB0 = new B();
	ptrB0->dump();
	delete ptrB0;

	std::tr1::shared_ptr<B> ptrB1(new B());

	ptrB1->dump();
	std::tr1::shared_ptr<B> ptrB2 = ptrB1;
		cout << "B1 count0: " << ptrB1.use_count() << endl;
		cout << "B2 count0: " << ptrB2.use_count() << endl;
	ptrB1.reset();
		cout << "B2 count1: " << ptrB2.use_count() << endl;
	ptrB2->dump();
	
	std::tr1::shared_ptr<B> ptrB3 = ptrB2;
		cout << "B2 count2: " << ptrB2.use_count() << endl;
		cout << "B3 count2: " << ptrB3.use_count() << endl;
	ptrB3->dump();
  return 0;
}



 