
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>

using namespace std;

struct Pair;
struct List;
struct Symbol;
struct Sundry;

typedef int16_t Int;

class Value
	{
	Int _bits;

	public:

	Value(const Pair &p);
	Value(const Symbol &s);
	Value(Int i);
	Value(const Sundry &s);

	bool  isPair()        const;
	bool  isList()        const;
	bool  isSymbol()      const;
	bool  isInt()         const;
	bool  isSundry()      const;

	Pair    & asPair()    const;
	List      asList()    const;
	Symbol  & asSymbol()  const;
	Int       asInt()     const;
	Sundry  & asSundry()  const;

	bool equal(const Value other) const;
	bool eq(const Value other) const { return _bits == other._bits; }

	friend ostream &operator<<(ostream &output, const Value &v);

	Value(){} // So the heap doesn't get initialized
	};

struct Pair
	{
	Value _car;
	Value _cdr;

	bool equal(const Pair &other) const;

	friend ostream &operator<<(ostream &output, const Pair &p);

	bool isList() const { return _cdr.isPair(); }
	};

struct List
	{
	const Pair &_pair;

	List(const Pair &p):_pair(p){}

	friend ostream &operator<<(ostream &output, const List &l);
	};

enum
	{
	HEAP_SIZE=10000,
	SYMBOL_TABLE_SIZE=1000,
	SUNDRY_TABLE_SIZE=1000,
	};

Pair heap[HEAP_SIZE];
Int numPairs = 0;

Pair &newPair(Value car, Value cdr)
	{
	Pair &result = heap[numPairs++];
	result._car = car;
	result._cdr = cdr;
	return result;
	}


const Pair &nil = newPair(heap[0], heap[0]); // Pair zero

Value car(Value parameters)
	{
	return parameters.asPair()._car;
	}

Value cdr(Value parameters)
	{
	return parameters.asPair()._cdr;
	}

typedef Value (*PrimitiveFunction)(Value parameters);

struct Symbol
	{
	const char *_name;
	PrimitiveFunction _implementation;

	bool equal(const Symbol &other) const { return this == &other; }

	friend ostream &operator<<(ostream &output, const Symbol &s)
      { 
		return output << s._name;
      }
	};

Symbol symbolTable[SYMBOL_TABLE_SIZE];
Int numSymbols = 0;

Symbol &newSymbol(const char *name, PrimitiveFunction f)
	{
	Symbol &result = symbolTable[numSymbols++];
	result._name = name;
	result._implementation = f;
	return result;
	}

enum SundryKind
	{
	SUNDRY_BOOLEAN,
	SUNDRY_SINGLETON,
	SUNDRY_ENVIRONMENT,
	};

enum SingletonKind
	{
	SINGLETON_IGNORE,
	SINGLETON_INERT,
	};

struct Sundry
	{
	SundryKind _tag;
	union
		{
		bool          _booleanValue;
		SingletonKind _singleton;
		};

	bool equal(const Sundry &other) const { return this == &other; }
	bool isA(SundryKind k) const { return _tag == k; }
	bool isSingleton(SingletonKind k) const { return isA(SUNDRY_SINGLETON) && _singleton == k; }
	};

Sundry sundryTable[SUNDRY_TABLE_SIZE];
Int numSundrys = 0;

Sundry &newBoolean(bool value)
	{
	Sundry &result = sundryTable[numSundrys++];
	result._tag = SUNDRY_BOOLEAN;
	result._booleanValue = value;
	return result;
	}

Sundry &newSingleton(SingletonKind k)
	{
	Sundry &result = sundryTable[numSundrys++];
	result._tag = SUNDRY_SINGLETON;
	result._singleton = k;
	return result;
	}

enum Tag
	{
	TAG_SUNDRY = 0,
	TAG_INT    = 1,
	TAG_PAIR   = 2,
	TAG_SYMBOL = 3,
	};

static inline Int tagged(Int bits, Tag tag)
	{
	return (bits << 2) | tag;
	}

static inline Int untagged(Int bits)
	{
	return bits >> 2;
	}

static inline Tag getTag(Int bits)
	{
	return (Tag)(bits & 3);
	}

inline Value::Value(const Pair &p):_bits(tagged(&p - heap, TAG_PAIR)){}
inline Value::Value(const Symbol &s):_bits(tagged(&s - symbolTable, TAG_SYMBOL)){}
inline Value::Value(Int i):_bits(tagged(i, TAG_INT)){}
inline Value::Value(const Sundry &s):_bits(tagged(&s - sundryTable, TAG_SUNDRY)){}

inline Pair &Value::asPair() const { assert(isPair()); return heap[untagged(_bits)]; }
inline List  Value::asList() const { assert(isList()); return List(asPair()); }
inline Symbol &Value::asSymbol() const { assert(isSymbol()); return symbolTable[untagged(_bits)]; }
inline Int Value::asInt() const { assert(isInt()); return untagged(_bits); }
inline Sundry &Value::asSundry() const { assert(isSundry()); return sundryTable[untagged(_bits)]; }

inline bool Value::isPair() const { return getTag(_bits) == TAG_PAIR; }
inline bool Value::isList() const { return isPair() && asPair().isList(); }
inline bool Value::isSymbol() const { return getTag(_bits) == TAG_SYMBOL; }
inline bool Value::isInt() const { return getTag(_bits) == TAG_INT; }
inline bool Value::isSundry() const { return getTag(_bits) == TAG_SUNDRY; }

bool Value::equal(const Value other) const
	{
	if (getTag(_bits) != getTag(other._bits))
		return false;
	else if (isPair())
		return asPair().equal(other.asPair());
	else if (isInt())
		return asInt() == other.asInt();
	else if (isSymbol())
		return asSymbol().equal(other.asSymbol());
	else
		return asSundry().equal(other.asSundry());
	}

bool Pair::equal(const Pair &other) const
	{
	// TODO: Terminate for cyclic structures
	return _car.equal(other._car) && _cdr.equal(other._cdr);
	}

ostream &operator<<(ostream &output, const Value &v)
	{ 
	if (v.isPair())
		return output << v.asPair();
	else if (v.isSymbol())
		return output << v.asSymbol();
	else
		{
		assert(v.isInt());
		return output << v.asInt();
		}
	}

static inline bool isNil(const Value v)
	{
	return &nil == &(v.asPair());
	}

ostream &operator<<(ostream &output, const Pair &p)
	{ 
	if (p.isList())
		return output << "(" << List(p) << ")";
	else
		return output << "(" << p._car << " . " << p._cdr << ")";
	}

ostream &operator<<(ostream &output, const List &l)
	{
	// When you get in here, your caller has decided to take care of any dot or
	// enclosing parens already.  You just print your contents.
	//
	output << l._pair._car;
	if (isNil(l._pair._cdr))
		return output;
	else if (l._pair._cdr.isList())
		return output << " " << l._pair._cdr.asList();
	else
		return output << " . " << l._pair._cdr;
	}

// 4.1  Booleans

const Sundry &FALSE = newBoolean(false);
const Sundry &TRUE  = newBoolean(true);

static inline const Sundry &BOOLEAN(bool which)
	{
	return which? TRUE : FALSE;
	}

static inline const Sundry &NOT(Sundry &b)
	{
	return BOOLEAN(!b._booleanValue);
	}

Value booleanP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isSundry() && parameters.asSundry().isA(SUNDRY_BOOLEAN));
	}
const Symbol &booleanP = newSymbol("boolean?", booleanP_primitive);

// 4.2  Equivalence under mutation (optional)

Value eqP_primitive(Value parameters)
	{
	Pair &args = parameters.asPair();
	Value object1 = args._car;
	Value object2 = args._cdr.asPair()._car;
	return BOOLEAN(object1.eq(object2));
	}
const Symbol &eqP = newSymbol("eq?", eqP_primitive);

// 4.3  Equivalence up to mutation

Value equalP_primitive(Value parameters)
	{
	Pair &args = parameters.asPair();
	Value object1 = args._car;
	Value object2 = args._cdr.asPair()._car;
	return BOOLEAN(object1.equal(object2));
	}
const Symbol &equalP = newSymbol("equal?", equalP_primitive);

// 4.4  Symbols

Value symbolP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isSymbol());
	}
const Symbol &symbolP = newSymbol("symbol?", symbolP_primitive);

// 4.5  Control

const Sundry &INERT = newSingleton(SINGLETON_INERT);

Value inertP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isSundry() && parameters.asSundry().isSingleton(SINGLETON_INERT));
	}
const Symbol &inertP = newSymbol("inert?", inertP_primitive);

Value Sif_primitive(Value parameters)
	{
	// TODO: eval the first one, then eval one of the other two
	return 0xdead;
	}

// 4.6  Pairs and lists

Value pairP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isPair());
	}
const Symbol &pairP = newSymbol("pair?", pairP_primitive);

Value nullP_primitive(Value parameters)
	{
	return BOOLEAN(isNil(parameters));
	}
const Symbol &nullP = newSymbol("null?", nullP_primitive);

Value cons_primitive(Value parameters)
	{
	Pair &args = parameters.asPair();
	return newPair(args._car, args._cdr.asPair()._car);
	}
const Symbol &cons = newSymbol("cons", cons_primitive);

// 4.7  Pair mutation (optional)


// 4.8  Environments

Value environmentP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isSundry() && parameters.asSundry().isA(SUNDRY_ENVIRONMENT));
	}

const Sundry &IGNORE = newSingleton(SINGLETON_IGNORE);

Value ignoreP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.isSundry() && parameters.asSundry().isSingleton(SINGLETON_IGNORE));
	}
const Symbol &ignoreP = newSymbol("ignore?", ignoreP_primitive);

Value eval_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &evalP = newSymbol("eval", eval_primitive);

Value make_environment_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &make_environmentP = newSymbol("make-environment", make_environment_primitive);

// 4.9  Environment mutation (optional)


// 4.10  Combiners

Value operativeP_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &operativeP = newSymbol("operative?", operativeP_primitive);

Value applicativeP_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &applicativeP = newSymbol("applicative?", applicativeP_primitive);

Value Svau_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &Svau = newSymbol("$vau", Svau_primitive);

Value wrap_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &wrap = newSymbol("wrap", wrap_primitive);

Value unwrap_primitive(Value parameters)
	{
	return 0xdead;
	}
const Symbol &unwrap = newSymbol("unwrap", unwrap_primitive);



// MAIN

int main(int argc, char *argv[])
	{
	Pair &list23 = newPair(2, newPair(3, nil));
	cout << cons_primitive(newPair(1, newPair(list23, nil))) << endl;
	cout << numPairs << " pairs, " << numSymbols << " symbols, " << numSundrys << " sundry" << endl;
	return 0;
	}

