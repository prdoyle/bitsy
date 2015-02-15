package main

import "fmt"

type Int int16
type Value int16
type Pair int16

//
// Values are a low-tagged union of Int-sized types representing a reference to
// a value that can be computed by Lisp
//

type Valuer interface {
	Value() Value
}

func (i Int) Value() Value  { return Value(INT) + Value(i<<TAG_SHIFT) }
func (p Pair) Value() Value { return Value(PAIR) + Value(p<<TAG_SHIFT) }

func (v Value) tag() Tag { return Tag(v & TAG_MASK) }

func (v Value) is_int() bool  { return v.tag() == INT }
func (v Value) is_pair() bool { return v.tag() == PAIR }

func (v Value) as_int() Int {
	assert("as_int requires an Int value", v.is_int())
	return Int(v >> TAG_SHIFT)
}

func (v Value) as_pair() Pair {
	assert("as_pair requires a Pair value", v.is_pair())
	return Pair(v >> TAG_SHIFT)
}

type Tag Int

const (
	PAIR Tag = iota
	INT

	TAG_SHIFT = 1
	TAG_MASK  = 1
)

func (t Tag) String() string {
	return [...]string{"PAIR", "INT"}[t]
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

func (i Int) String() string { return fmt.Sprintf("%d", i) }

func (p Pair) String() string {
	return fmt.Sprintf("(%s)", p.list_contents())
}

func (p Pair) list_contents() string {
	guts := global_state._pair_table[p]
	if p == 0 {
		return ""
	} else if guts._cdr == 0 {
		return guts._car.String()
	} else if guts._cdr.is_pair() {
		return fmt.Sprintf("%s %s", guts._car, guts._cdr.as_pair().list_contents())
	} else {
		return fmt.Sprintf("%s . %s", guts._car, guts._cdr)
	}
}

//
// Pairs
//

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
func (ps *Program_state) supply_raw_pairs() {
	for true {
		index := len(ps._pair_table)
		ps._pair_table = append(ps._pair_table, pair_guts{0, 0})
		ps._raw_pairs <- Pair(index)
	}
}

//
// main
//

var global_state = new_program_state()

func main() {
	ps := global_state
	my_list := ps.int_list([]Int{1, 2, 3, 4})
	ps.dump_pairs()
	fmt.Println(my_list)
}

type Program_state struct {
	_pair_table []pair_guts
	_raw_pairs  chan Pair
	_nil        Value
}

func new_program_state() *Program_state {
	result := new(Program_state)
	result._pair_table = make([]pair_guts, 0)
	result._raw_pairs = make(chan Pair)
	go result.supply_raw_pairs()
	result._nil = result.new_pair(Pair(0).Value(), Pair(0).Value()).Value()
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

func (ps *Program_state) dump_pairs() {
	for i, guts := range ps._pair_table {
		fmt.Printf("[%d]: %#v\n", i, guts)
	}
}

//
// Yes, I want assertions dammit
//

func assert(message string, condition bool) {
	if !condition {
		panic(message)
	}
}
