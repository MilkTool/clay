from clay.core import *



#
# type patterns
#

class Pattern(object) :
    pass

class Cell(Pattern) :
    def __init__(self, name=None) :
        super(Cell, self).__init__()
        self.name = name
        # value is a Value
        self.value = None

class TypePattern(Pattern) :
    pass

class PointerTypePattern(TypePattern) :
    def __init__(self, pointeeType) :
        super(PointerTypePattern, self).__init__()
        # pointeeType is Value|Pattern
        self.pointeeType = pointeeType

class TupleTypePattern(TypePattern) :
    def __init__(self, elementTypes) :
        super(TupleTypePattern, self).__init__()
        # elementTypes is a list of Value|Pattern
        self.elementTypes = elementTypes

class ArrayTypePattern(TypePattern) :
    def __init__(self, elementType, size) :
        super(ArrayTypePattern, self).__init__()
        # elementType is a Value|Pattern
        self.elementType = elementType
        # size is a Value|Cell
        self.size = size

class RecordTypePattern(TypePattern) :
    def __init__(self, record, params) :
        super(RecordTypePattern, self).__init__()
        # record is a Record
        self.record = record
        # params is a list of Value|Pattern
        self.params = params



#
# pattern matching by unification
#

unify = multimethod("unify", n=2)

@unify.register(Value, Value)
def foo(a, b) :
    return a == b

@unify.register(Cell, Value)
def foo(a, b) :
    if a.value is None :
        a.value = b
        return True
    return unify(a.value, b)

@unify.register(TypePattern, Value)
def foo(a, b) :
    if b.type != compilerObjectType :
        return False
    return unifyType(a, fromCOValue(b))

unifyType = multimethod("unifyType", n=2)

@unifyType.register(TypePattern, object)
def foo(a, b) :
    # b can also be a non-type compiler object
    return False

@unifyType.register(PointerTypePattern, PointerType)
def foo(a, b) :
    return unify(a.pointeeType, toCOValue(b.pointeeType))

@unifyType.register(TupleTypePattern, TupleType)
def foo(a, b) :
    if len(a.elementTypes) != len(b.elementTypes) :
        return False
    for x, y in zip(a.elementTypes, b.elementTypes) :
        if not unify(x, toCOValue(y)) :
            return False
    return True

@unifyType.register(ArrayTypePattern, ArrayType)
def foo(a, b) :
    if not unify(a.elementType, toCOValue(b.elementType)) :
        return False
    return unify(a.size, toInt32Value(b.size))

@unifyType.register(RecordTypePattern, RecordType)
def foo(a, b) :
    if a.record != b.record :
        return False
    for x, y in zip(a.params, b.params) :
        if not unify(x, y) :
            return False
    return True

resolvePattern = multimethod("resolvePattern")

@resolvePattern.register(Value)
def foo(x) :
    return x

@resolvePattern.register(Cell)
def foo(x) :
    if x.value is None :
        error("unresolved pattern variable: " + x.name)
    return x.value

@resolvePattern.register(PointerTypePattern)
def foo(x) :
    pointeeType = toType(resolvePattern(x.pointeeType))
    return toCOValue(pointerType(pointeeType))

@resolvePattern.register(TupleTypePattern)
def foo(x) :
    elementTypes = [toType(resolvePattern(y)) for y in x.elementTypes]
    return toCOValue(tupleType(elementTypes))

@resolvePattern.register(ArrayTypePattern)
def foo(x) :
    elementType = toType(resolvePattern(x.elementType))
    size = fromIntValue(resolvePattern(x.size))
    return toCOValue(arrayType(elementType, size))

@resolvePattern.register(RecordTypePattern)
def foo(x) :
    params = [resolvePattern(y) for y in x.params]
    return toCOValue(recordType(x.record, params))



#
# remove temp name used for multimethod instances
#

del foo
