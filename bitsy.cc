
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>

using namespace std;

struct Pair;
struct List;
struct Symbol;
struct Sundry;

typedef int8_t Int;

class Value
	{
	Int _bits;

	public:

	Value(const Pair &p);
	Value(const Symbol &s);
	Value(Int i);
	Value(const Sundry &s);

	bool  is_pair()        const;
	bool  is_list()        const;
	bool  is_symbol()      const;
	bool  is_int()         const;
	bool  is_sundry()      const;

	Pair    & as_pair()    const;
	List      as_list()    const;
	Symbol  & as_symbol()  const;
	Int       as_int()     const;
	Sundry  & as_sundry()  const;

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

	bool is_list() const { return _cdr.is_pair(); }
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
Int num_pairs = 0;

Pair &new_pair(Value car, Value cdr);
Pair &new_pair(Value car, Value cdr)
	{
	Pair &result = heap[num_pairs++];
	result._car = car;
	result._cdr = cdr;
	return result;
	}


const Pair &nil = new_pair(heap[0], heap[0]); // Pair zero

static inline bool is_nil(const Value v)
	{
	return &nil == &(v.as_pair());
	}

Value car(Value parameters)
	{
	return parameters.as_pair()._car;
	}

Value cdr(Value parameters)
	{
	return parameters.as_pair()._cdr;
	}

typedef Value (*Primitive_function)(Value parameters);

struct Symbol
	{
	// This definition makes no sense.  Names should be bound to implementations.
	// I'm missing an abstractions for combiners or procedures or something.

	const char *_name;
	Primitive_function _implementation;

	bool equal(const Symbol &other) const { return this == &other; }

	friend ostream &operator<<(ostream &output, const Symbol &s)
      { 
		return output << s._name;
      }
	};

Symbol symbol_table[SYMBOL_TABLE_SIZE];
Int num_symbols = 0;

Symbol &new_symbol(const char *name, Primitive_function f)
	{
	Symbol &result = symbol_table[num_symbols++];
	result._name = name;
	result._implementation = f;
	return result;
	}

enum Sundry_kind
	{
	SUNDRY_BOOLEAN,
	SUNDRY_SINGLETON,
	SUNDRY_ENVIRONMENT,
	};

enum Singleton_kind
	{
	SINGLETON_IGNORE,
	SINGLETON_INERT,
	};

struct Environment
	{
	Value        _bindings;
	Environment *_outer;
	};

struct Sundry
	{
	Sundry_kind _tag;
	union
		{
		bool           _boolean_value;
		Singleton_kind _singleton;
		Environment    _environment;
		};

	bool equal(const Sundry &other) const { return this == &other; }
	bool is_a(Sundry_kind k) const { return _tag == k; }
	bool is_singleton(Singleton_kind k) const { return is_a(SUNDRY_SINGLETON) && _singleton == k; }

	Sundry(){} // So I can declare an uninitialized array
	};

Sundry sundry_table[SUNDRY_TABLE_SIZE];
Int num_sundrys = 0;

Sundry &new_boolean(bool value)
	{
	Sundry &result = sundry_table[num_sundrys++];
	result._tag = SUNDRY_BOOLEAN;
	result._boolean_value = value;
	return result;
	}

Sundry &new_singleton(Singleton_kind k)
	{
	Sundry &result = sundry_table[num_sundrys++];
	result._tag = SUNDRY_SINGLETON;
	result._singleton = k;
	return result;
	}

Sundry &new_environment(Environment *outer)
	{
	Environment blank_environment_initializer = { nil, outer };
	Sundry &result = sundry_table[num_sundrys++];
	result._tag = SUNDRY_ENVIRONMENT;
	result._environment = blank_environment_initializer;
	return result;
	}

enum Tag
	{
	TAG_SUNDRY = 0,
	TAG_INT    = 1,
	TAG_PAIR   = 2,
	TAG_SYMBOL = 3,

	TAG_NUM_BITS = 2,
	TAG_MASK     = 3,
	};

static inline Int tagged(Int bits, Tag tag)
	{
	return (bits << TAG_NUM_BITS) | tag;
	}

static inline Int untagged(Int bits)
	{
	return bits >> TAG_NUM_BITS;
	}

static inline Tag get_tag(Int bits)
	{
	return (Tag)(bits & TAG_MASK);
	}

inline Value::Value(const Pair &p):_bits(tagged(&p - heap, TAG_PAIR)){}
inline Value::Value(const Symbol &s):_bits(tagged(&s - symbol_table, TAG_SYMBOL)){}
inline Value::Value(Int i):_bits(tagged(i, TAG_INT)){}
inline Value::Value(const Sundry &s):_bits(tagged(&s - sundry_table, TAG_SUNDRY)){}

inline Pair &Value::as_pair() const { assert(is_pair()); return heap[untagged(_bits)]; }
inline List  Value::as_list() const { assert(is_list()); return List(as_pair()); }
inline Symbol &Value::as_symbol() const { assert(is_symbol()); return symbol_table[untagged(_bits)]; }
inline Int Value::as_int() const { assert(is_int()); return untagged(_bits); }
inline Sundry &Value::as_sundry() const { assert(is_sundry()); return sundry_table[untagged(_bits)]; }

inline bool Value::is_pair() const { return get_tag(_bits) == TAG_PAIR; }
inline bool Value::is_list() const { return is_pair() && as_pair().is_list(); }
inline bool Value::is_symbol() const { return get_tag(_bits) == TAG_SYMBOL; }
inline bool Value::is_int() const { return get_tag(_bits) == TAG_INT; }
inline bool Value::is_sundry() const { return get_tag(_bits) == TAG_SUNDRY; }

bool Value::equal(const Value other) const
	{
	if (get_tag(_bits) != get_tag(other._bits))
		return false;
	else if (is_pair())
		return as_pair().equal(other.as_pair());
	else if (is_int())
		return as_int() == other.as_int();
	else if (is_symbol())
		return as_symbol().equal(other.as_symbol());
	else
		return as_sundry().equal(other.as_sundry());
	}

bool Pair::equal(const Pair &other) const
	{
	// TODO: Terminate for cyclic structures
	return _car.equal(other._car) && _cdr.equal(other._cdr);
	}

ostream &operator<<(ostream &output, const Value &v)
	{ 
	if (v.is_pair())
		return output << v.as_pair();
	else if (v.is_symbol())
		return output << v.as_symbol();
	else
		{
		// If Int is defined as char, then C++ streams become terribly vexed.
		// Cast them as int when they are smaller than ints.
		//
		Int i = v.as_int();
		if (sizeof(i) < sizeof(int))
			return output << (int)i;
		else
			return output << i;
		}
	}

ostream &operator<<(ostream &output, const Pair &p)
	{ 
	if (p.is_list())
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
	if (is_nil(l._pair._cdr))
		return output;
	else if (l._pair._cdr.is_list())
		return output << " " << l._pair._cdr.as_list();
	else
		return output << " . " << l._pair._cdr;
	}

static Value whoopsie()
	{
	assert(0);
	return 0xdd;
	}

// 4.1  Booleans

const Sundry &FALSE = new_boolean(false);
const Sundry &TRUE  = new_boolean(true);

static inline const Sundry &BOOLEAN(bool which)
	{
	return which? TRUE : FALSE;
	}

static inline const Sundry &NOT(Sundry &b)
	{
	return BOOLEAN(!b._boolean_value);
	}

Value booleanP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_sundry() && parameters.as_sundry().is_a(SUNDRY_BOOLEAN));
	}
const Symbol &booleanP = new_symbol("boolean?", booleanP_primitive);

// 4.2  Equivalence under mutation (optional)

Value eqP_primitive(Value parameters)
	{
	Pair &args = parameters.as_pair();
	Value object1 = args._car;
	Value object2 = args._cdr.as_pair()._car;
	return BOOLEAN(object1.eq(object2));
	}
const Symbol &eqP = new_symbol("eq?", eqP_primitive);

// 4.3  Equivalence up to mutation

Value equalP_primitive(Value parameters)
	{
	Pair &args = parameters.as_pair();
	Value object1 = args._car;
	Value object2 = args._cdr.as_pair()._car;
	return BOOLEAN(object1.equal(object2));
	}
const Symbol &equalP = new_symbol("equal?", equalP_primitive);

// 4.4  Symbols

Value symbolP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_symbol());
	}
const Symbol &symbolP = new_symbol("symbol?", symbolP_primitive);

// 4.5  Control

const Sundry &INERT = new_singleton(SINGLETON_INERT);

Value inertP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_sundry() && parameters.as_sundry().is_singleton(SINGLETON_INERT));
	}
const Symbol &inertP = new_symbol("inert?", inertP_primitive);

Value Sif_primitive(Value parameters)
	{
	// TODO: eval the first one, then eval one of the other two
	return whoopsie();
	}

// 4.6  Pairs and lists

Value pairP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_pair());
	}
const Symbol &pairP = new_symbol("pair?", pairP_primitive);

Value nullP_primitive(Value parameters)
	{
	return BOOLEAN(is_nil(parameters));
	}
const Symbol &nullP = new_symbol("null?", nullP_primitive);

Value cons_primitive(Value parameters)
	{
	Pair &args = parameters.as_pair();
	return new_pair(args._car, args._cdr.as_pair()._car);
	}
const Symbol &cons = new_symbol("cons", cons_primitive);

// 4.7  Pair mutation (optional)


// 4.8  Environments

Pair &prepend_binding(Pair &bindings, Value key, Value value)
	{
	return new_pair(
		new_pair(key, value),
		bindings);
	}

Value lookup(const Environment *env, Value key);

Value lookup_impl(const Environment *env, const Pair &bindings, Value key)
	{
	if (is_nil(bindings))
		return lookup(env->_outer, key);
	else if (bindings._car.as_pair()._car.eq(key))
		return bindings._car.as_pair()._cdr;
	else
		return lookup_impl(env, bindings._cdr.as_pair(), key);
	}

Value lookup(const Environment *env, Value key)
	{
	if (env)
		return lookup_impl(env, env->_bindings.as_pair(), key);
	else
		return whoopsie();
	}

Value environmentP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_sundry() && parameters.as_sundry().is_a(SUNDRY_ENVIRONMENT));
	}

const Sundry &IGNORE = new_singleton(SINGLETON_IGNORE);

Value ignoreP_primitive(Value parameters)
	{
	return BOOLEAN(parameters.is_sundry() && parameters.as_sundry().is_singleton(SINGLETON_IGNORE));
	}
const Symbol &ignoreP = new_symbol("ignore?", ignoreP_primitive);

Value eval_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &evalP = new_symbol("eval", eval_primitive);

Value make_environment_primitive(Value parameters)
	{
	return new_environment(NULL);
	}
const Symbol &make_environmentP = new_symbol("make-environment", make_environment_primitive);

// 4.9  Environment mutation (optional)


// 4.10  Combiners

Value operativeP_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &operativeP = new_symbol("operative?", operativeP_primitive);

Value applicativeP_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &applicativeP = new_symbol("applicative?", applicativeP_primitive);

Value Svau_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &Svau = new_symbol("$vau", Svau_primitive);

Value wrap_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &wrap = new_symbol("wrap", wrap_primitive);

Value unwrap_primitive(Value parameters)
	{
	return whoopsie();
	}
const Symbol &unwrap = new_symbol("unwrap", unwrap_primitive);



// MAIN

int main(int argc, char *argv[])
	{
	Pair &list23 = new_pair(2, new_pair(3, nil));
	cout << cons_primitive(new_pair(1, new_pair(list23, nil))) << endl;
	cout << (Value)num_pairs << " pairs, " << (Value)num_symbols << " symbols, " << (Value)num_sundrys << " sundry" << endl;
	return 0;
	}

