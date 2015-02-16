package main

// Bitsy is an interpreter for the Kernel dialect of Lisp, designed to use tiny
// quantities of memory when possible by arranging its data in tables and using
// indexes instead of pointers.

import "fmt"

//
// Bitsy is designed to avoid pointers most of the time in order to save memory.
//
// Make this bigger to run bigger programs
//
type Int int16

func (i Int) String() string { return fmt.Sprintf("%d", i) }

//
// Values are a low-tagged union of Int-sized types representing a reference to
// a value that can be computed by Lisp
//
type Value int16

func (v Value) tag() Tag { return Tag(v & TAG_MASK) }

type Tag Int

const (
	INT Tag = iota
	PAIR
	SYMBOL
	SUNDRY // All the other kinds of Values that are not quite so numerous or fundamental

	TAG_SHIFT = 2
	TAG_MASK  = 3
)

func (i Int) Value() Value    { return Value(i<<TAG_SHIFT) + Value(INT) }
func (p Pair) Value() Value   { return Value(p<<TAG_SHIFT) + Value(PAIR) }
func (s Symbol) Value() Value { return Value(s<<TAG_SHIFT) + Value(SYMBOL) }
func (x Sundry) Value() Value { return Value(x<<TAG_SHIFT) + Value(SUNDRY) }

func (v Value) is_int() bool    { return v.tag() == INT }
func (v Value) is_pair() bool   { return v.tag() == PAIR }
func (v Value) is_symbol() bool { return v.tag() == SYMBOL }
func (v Value) is_sundry() bool { return v.tag() == SUNDRY }

func (v Value) as_int() Int {
	assert("as_int requires an Int value", v.is_int())
	return Int(v >> TAG_SHIFT)
}

func (v Value) as_pair() Pair {
	assert("as_pair requires a Pair value", v.is_pair())
	return Pair(v >> TAG_SHIFT)
}

func (v Value) as_symbol() Symbol {
	assert("as_symbol requires a Symbol value", v.is_symbol())
	return Symbol(v >> TAG_SHIFT)
}
func (v Value) as_sundry() Sundry {
	assert("as_sundry requires a Sundry value", v.is_sundry())
	return Sundry(v >> TAG_SHIFT)
}

func (v Value) is_nil() bool {
	return v.is_pair() && v.as_pair() == 0
}

func (t Tag) String() string {
	return [...]string{"INT", "PAIR", "SYMBOL", "SUNDRY"}[t]
}

func (v Value) String() string {
	if v.is_int() {
		return v.as_int().String()
	} else if v.is_pair() {
		return v.as_pair().String()
	} else {
		panic("Hey what kind of value is v??")
	}
}

//
// Pairs
//
type Pair Int // An index into the _pair_table

type pair_guts struct {
	_car Value
	_cdr Value
}

func car(p Pair) Value { return global_state._pair_table[p]._car }
func cdr(p Pair) Value { return global_state._pair_table[p]._cdr }

func (ps *Program_state) new_pair(car Value, cdr Value) Pair {
	result := <-ps._raw_pairs
	guts := &ps._pair_table[result]
	guts._car = car
	guts._cdr = cdr
	return result
}

// We allocate pairs in a goroutine to serialize the allocation.
//
// TODO: This code actually always allocates the next pair before it's needed.
// I wonder if this could be made to create only precisely as many as are
// needed?  Or is this a funtamental flaw with the approach?
//
func (ps *Program_state) supply_raw_pairs() {
	for true {
		index := len(ps._pair_table)
		ps._pair_table = append(ps._pair_table, pair_guts{0, 0})
		ps._raw_pairs <- Pair(index)
	}
}

func (p Pair) String() string {
	return fmt.Sprintf("(%s)", p.list_contents())
}

func (p Pair) list_contents() string {
	verbose("--list_contents on %#v", p)
	guts := global_state._pair_table[p]
	if p == 0 {
		verbose("  --nil")
		return ""
	} else if guts._cdr.is_nil() {
		verbose("  --cdr is nil")
		return guts._car.String()
	} else if guts._cdr.is_pair() {
		verbose("  --continuing list")
		return fmt.Sprintf("%s %s", guts._car, guts._cdr.as_pair().list_contents())
	} else {
		verbose("  --printing as pair")
		return fmt.Sprintf("%s . %s", guts._car, guts._cdr)
	}
}

//
// Symbols
//
type Symbol Int

type symbol_guts struct {
	_name string
}

func (s Symbol) name() string { return global_state._symbol_table[s]._name }

func (ps *Program_state) new_symbol(name string) Symbol {
	result := <-ps._raw_symbols
	guts := &ps._symbol_table[result]
	guts._name = name
	return result
}

func (ps *Program_state) supply_raw_symbols() {
	for true {
		index := len(ps._symbol_table)
		ps._symbol_table = append(ps._symbol_table, symbol_guts{""})
		ps._raw_symbols <- Symbol(index)
	}
}

func (s Symbol) String() string {
	return global_state._symbol_table[s]._name
}

//
// Sundry
//
type Sundry Int

func (x Sundry) kind() Sundry_kind { return Sundry_kind(x & SUNDRY_MASK) }

type Sundry_kind Int

const (
	SUNDRY_BOOLEAN Sundry_kind = iota<<TAG_SHIFT + Sundry_kind(SUNDRY)
	SUNDRY_SINGLETON
	SUNDRY_ENV

	SUNDRY_SHIFT = 2 + TAG_SHIFT
	SUNDRY_MASK  = 3<<TAG_SHIFT + TAG_MASK
)

func (e Env) Value() Value            { return Value(e<<SUNDRY_SHIFT) + Value(SUNDRY_ENV) }
func bool_value(b bool) Value         { return bool_to_sundry(b) + Value(SUNDRY_BOOLEAN) }
func (s Singleton_kind) Value() Value { return Value(s<<SINGLETON_SHIFT) + Value(SUNDRY_SINGLETON) }

func (v Value) is_boolean() bool   { return Sundry_kind(v)&SUNDRY_MASK == SUNDRY_BOOLEAN }
func (v Value) is_singleton() bool { return Sundry_kind(v)&SUNDRY_MASK == SUNDRY_SINGLETON }
func (v Value) is_env() bool       { return Sundry_kind(v)&SUNDRY_MASK == SUNDRY_ENV }

func (v Value) as_boolean() bool {
	assert("as_boolean requies a boolean value", v.is_boolean())
	return (v >> SUNDRY_SHIFT) != 0
}

func (v Value) as_singleton() Singleton_kind {
	assert("as_singleton requies a singleton value", v.is_singleton())
	return Singleton_kind(v & SINGLETON_MASK)
}

func (v Value) as_env() Env {
	assert("as_env requies a env value", v.is_env())
	return Env(v >> SUNDRY_SHIFT)
}

func bool_to_sundry(b bool) Value {
	if b {
		return Value(1 << SUNDRY_SHIFT)
	} else {
		return 0
	}
}

type Singleton_kind Int

const (
	SINGLETON_IGNORE Singleton_kind = iota << SUNDRY_SHIFT
	SINGLETON_INERT

	SINGLETON_SHIFT = 1 + SUNDRY_SHIFT
	SINGLETON_MASK  = 1 << SUNDRY_SHIFT
)

//
// Env
//

type Env Int

type env_guts struct {
	_defs map[Symbol]Value
}

func (e Env) get(s Symbol) Value {
	guts := &global_state._env_table[e]
	result, ok := guts._defs[s]
	if ok {
		return result
	} else {
		panic(fmt.Sprintf("Undefined symbol %#v", s.name()))
	}
}

func (e Env) set(s Symbol, v Value) {
	guts := &global_state._env_table[e]
	guts._defs[s] = v
}

func (ps *Program_state) new_env() Env {
	result := <-ps._raw_envs
	guts := &ps._env_table[result]
	guts._defs = make(map[Symbol]Value)
	return result
}

func (ps *Program_state) supply_raw_envs() {
	for true {
		index := len(ps._env_table)
		ps._env_table = append(ps._env_table, env_guts{})
		ps._raw_envs <- Env(index)
	}
}

//
// global program state
//

var global_state = new_program_state()

type Program_state struct {
	_pair_table   []pair_guts
	_raw_pairs    chan Pair
	_symbol_table []symbol_guts
	_raw_symbols  chan Symbol
	_env_table    []env_guts
	_raw_envs     chan Env
	_nil          Value
}

func new_program_state() *Program_state {
	result := new(Program_state)

	// Check out the boilerplate here.  Kinda gross.

	result._pair_table = make([]pair_guts, 1)
	result._raw_pairs = make(chan Pair, 0)
	go result.supply_raw_pairs()

	result._symbol_table = make([]symbol_guts, 0)
	result._raw_symbols = make(chan Symbol, 0)
	go result.supply_raw_symbols()

	result._env_table = make([]env_guts, 0)
	result._raw_envs = make(chan Env, 0)
	go result.supply_raw_envs()

	result._nil = Pair(0).Value()
	assert("nil must be Pair(0)", result._nil.as_pair() == 0)
	return result
}

func (ps *Program_state) int_list(ints []Int) Value {
	if len(ints) == 0 {
		return ps._nil
	} else {
		first := Int(ints[0]).Value()
		rest := ps.int_list(ints[1:])
		return ps.new_pair(first, rest).Value()
	}
}

func (ps *Program_state) dump() {
	fmt.Println("Pairs:")
	for i, guts := range ps._pair_table {
		fmt.Printf("[%d]: %#v\n", i, guts)
	}
	fmt.Println("Symbols:")
	for i, guts := range ps._symbol_table {
		fmt.Printf("[%d]: %#v\n", i, guts)
	}
	fmt.Println("Envs:")
	for i, guts := range ps._env_table {
		fmt.Printf("[%d]: %#v\n", i, guts)
	}
}

//
// main
//

func main() {
	ps := global_state
	my_list := ps.int_list([]Int{1, 2, 3, 4})
	env := ps.new_env()
	sym := ps.new_symbol("my_list")
	env.set(sym, my_list)
	ps.dump()
	fmt.Println(env.get(sym))
}

//
// Yes, I want assertions dammit
//

func assert(message string, condition bool) {
	if !condition {
		panic(message)
	}
}

//
// Debugging
//

func verbose(format string, args ...interface{}) {
	//fmt.Printf(format+"\n", args...)
}
