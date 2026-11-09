"""The few libclang C API calls gxcalls.py needs, through ctypes (no Python bindings required).

Uses the system libclang (Fedora: clang-libs, /usr/lib64/libclang.so). Set LIBCLANG to override.
"""

from __future__ import annotations

import ctypes as C
import ctypes.util
import os


class CXString(C.Structure):
    _fields_ = [("data", C.c_void_p), ("flags", C.c_uint)]


class CXCursor(C.Structure):
    _fields_ = [("kind", C.c_int), ("xdata", C.c_int), ("data", C.c_void_p * 3)]


class CXSourceLocation(C.Structure):
    _fields_ = [("ptr", C.c_void_p * 2), ("int_data", C.c_uint)]


class CXSourceRange(C.Structure):
    _fields_ = [("ptr", C.c_void_p * 2), ("begin", C.c_uint), ("end", C.c_uint)]


class CXToken(C.Structure):
    _fields_ = [("int_data", C.c_uint * 4), ("ptr", C.c_void_p)]


VISITOR = C.CFUNCTYPE(C.c_int, CXCursor, CXCursor, C.c_void_p)

# CXCursorKind values used here
STRUCT, CLASS, ENUM_CONST, FUNCTION, NAMESPACE, METHOD, CTOR, DTOR, FUNC_TEMPLATE, CLASS_TEMPLATE = 2, 4, 7, 8, 22, 21, 24, 25, 30, 31
DECL_REF, CALL = 101, 103
IF, SWITCH, WHILE, DO, FOR, COND_OP = 205, 202, 207, 208, 209, 116
FUNC_KINDS = {FUNCTION, METHOD, CTOR, DTOR, FUNC_TEMPLATE}
SCOPE_KINDS = {NAMESPACE, STRUCT, CLASS, CLASS_TEMPLATE, 23}  # 23 = LinkageSpec... (harmless)
BRANCH_KINDS = {IF, SWITCH, WHILE, DO, FOR, COND_OP}

_lib = None


def lib():
    global _lib
    if _lib is None:
        path = os.environ.get("LIBCLANG") or ctypes.util.find_library("clang") or "/usr/lib64/libclang.so"
        L = C.CDLL(path)
        sig = {
            "clang_createIndex": ([C.c_int, C.c_int], C.c_void_p),
            "clang_parseTranslationUnit": ([C.c_void_p, C.c_char_p, C.POINTER(C.c_char_p), C.c_int, C.c_void_p, C.c_uint, C.c_uint], C.c_void_p),
            "clang_disposeTranslationUnit": ([C.c_void_p], None),
            "clang_getTranslationUnitCursor": ([C.c_void_p], CXCursor),
            "clang_visitChildren": ([CXCursor, VISITOR, C.c_void_p], C.c_uint),
            "clang_getCursorSpelling": ([CXCursor], CXString),
            "clang_getCString": ([CXString], C.c_char_p),
            "clang_disposeString": ([CXString], None),
            "clang_getCursorLocation": ([CXCursor], CXSourceLocation),
            "clang_getExpansionLocation": ([CXSourceLocation, C.POINTER(C.c_void_p), C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint)], None),
            "clang_getFileName": ([C.c_void_p], CXString),
            "clang_getCursorReferenced": ([CXCursor], CXCursor),
            "clang_getEnumConstantDeclValue": ([CXCursor], C.c_longlong),
            "clang_Cursor_getNumArguments": ([CXCursor], C.c_int),
            "clang_Cursor_getArgument": ([CXCursor, C.c_uint], CXCursor),
            "clang_Cursor_Evaluate": ([CXCursor], C.c_void_p),
            "clang_EvalResult_getKind": ([C.c_void_p], C.c_int),
            "clang_EvalResult_getAsLongLong": ([C.c_void_p], C.c_longlong),
            "clang_EvalResult_dispose": ([C.c_void_p], None),
            "clang_getCursorExtent": ([CXCursor], CXSourceRange),
            "clang_tokenize": ([C.c_void_p, CXSourceRange, C.POINTER(C.POINTER(CXToken)), C.POINTER(C.c_uint)], None),
            "clang_getTokenSpelling": ([C.c_void_p, CXToken], CXString),
            "clang_disposeTokens": ([C.c_void_p, C.POINTER(CXToken), C.c_uint], None),
            "clang_Cursor_isNull": ([CXCursor], C.c_int),
            "clang_isCursorDefinition": ([CXCursor], C.c_uint),
            "clang_getCursorUSR": ([CXCursor], CXString),
            "clang_getNumDiagnostics": ([C.c_void_p], C.c_uint),
            "clang_getDiagnostic": ([C.c_void_p, C.c_uint], C.c_void_p),
            "clang_getDiagnosticSeverity": ([C.c_void_p], C.c_int),
            "clang_getDiagnosticSpelling": ([C.c_void_p], CXString),
            "clang_disposeDiagnostic": ([C.c_void_p], None),
        }
        for name, (args, res) in sig.items():
            f = getattr(L, name)
            f.argtypes, f.restype = args, res
        _lib = L
    return _lib


def _s(cxs: CXString) -> str:
    L = lib()
    v = L.clang_getCString(cxs)
    L.clang_disposeString(cxs)
    return v.decode("utf-8", "replace") if v else ""


def spelling(c) -> str:
    return _s(lib().clang_getCursorSpelling(c))


def location(c):
    L = lib()
    f, line, col, off = C.c_void_p(), C.c_uint(), C.c_uint(), C.c_uint()
    L.clang_getExpansionLocation(L.clang_getCursorLocation(c), C.byref(f), C.byref(line), C.byref(col), C.byref(off))
    return (_s(L.clang_getFileName(f)) if f.value else ""), line.value


def children(c) -> list:
    out = []

    def visit(child, _parent, _data):
        out.append(child)
        return 1  # CXChildVisit_Continue

    cb = VISITOR(visit)
    lib().clang_visitChildren(c, cb, None)
    return out


def usr(c) -> str:
    return _s(lib().clang_getCursorUSR(c))


def is_definition(c) -> bool:
    return bool(lib().clang_isCursorDefinition(c))


def referenced(c):
    r = lib().clang_getCursorReferenced(c)
    return None if lib().clang_Cursor_isNull(r) else r


def enum_value(c) -> int:
    return lib().clang_getEnumConstantDeclValue(c)


def call_args(c) -> list:
    L = lib()
    return [L.clang_Cursor_getArgument(c, i) for i in range(max(L.clang_Cursor_getNumArguments(c), 0))]


def evaluate_int(c):
    L = lib()
    r = L.clang_Cursor_Evaluate(c)
    if not r:
        return None
    try:
        return L.clang_EvalResult_getAsLongLong(r) if L.clang_EvalResult_getKind(r) == 1 else None
    finally:
        L.clang_EvalResult_dispose(r)


def tokens(tu, c) -> str:
    L = lib()
    toks, n = C.POINTER(CXToken)(), C.c_uint()
    L.clang_tokenize(tu, L.clang_getCursorExtent(c), C.byref(toks), C.byref(n))
    try:
        return " ".join(_s(L.clang_getTokenSpelling(tu, toks[i])) for i in range(n.value))
    finally:
        L.clang_disposeTokens(tu, toks, n)


def parse(path: str, args: list[str]):
    L = lib()
    idx = L.clang_createIndex(0, 0)
    argv = (C.c_char_p * len(args))(*[a.encode() for a in args])
    # options: CXTranslationUnit_SkipFunctionBodies is NOT wanted; 0x200 = KeepGoing
    tu = L.clang_parseTranslationUnit(idx, path.encode(), argv, len(args), None, 0, 0x200)
    if not tu:
        raise RuntimeError(f"libclang failed to parse {path}")
    return tu, L.clang_getTranslationUnitCursor(tu)


def errors(tu) -> list[str]:
    """Error/fatal diagnostics of a parsed translation unit."""
    L = lib()
    out = []
    for i in range(L.clang_getNumDiagnostics(tu)):
        d = L.clang_getDiagnostic(tu, i)
        if L.clang_getDiagnosticSeverity(d) >= 3:
            out.append(_s(L.clang_getDiagnosticSpelling(d)))
        L.clang_disposeDiagnostic(d)
    return out
