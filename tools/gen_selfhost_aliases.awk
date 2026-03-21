function trim(s) {
    sub(/^[ \t\r\n]+/, "", s)
    sub(/[ \t\r\n]+$/, "", s)
    return s
}

function map_type(type_text,    t, inner) {
    t = trim(type_text)
    if (t == "REX" || t == "DUX" || t == "COMES" || t == "MILES" || t == "VERUM") return "long long"
    if (t == "UMBRA" || t == "FLAMMA") return "double"
    if (t == "VERBUM") return "const char *"
    if (t == "NIHIL") return "void"
    if (t == "SPECULUM NIHIL") return "void *"
    if (t == "SPECULUM REX") return "long long *"
    if (t == "SPECULUM SourceLocation") return "SourceLocation *"
    if (index(t, "SPECULUM ") == 1) {
        inner = substr(t, 10)
        return inner " *"
    }
    return t
}

function map_target_name(name) {
    if (name == "stringify_hex") return "stringify_int_hex"
    if (name == "parse_int") return "fmt_parse_int"
    if (name == "parse_real") return "fmt_parse_real"
    return name
}

BEGIN {
    ordinal = 0
    print ""
    print "/* ===== Selfhost Prelude ABI Aliases ===== */"
}

/^RITUALE [A-Za-z_][A-Za-z0-9_]*\(.*\) REDDE .*;$/ {
    line = $0
    sub(/^RITUALE /, "", line)

    open_paren = index(line, "(")
    close_paren = index(line, ")")
    if (open_paren <= 0 || close_paren <= open_paren) next

    name = substr(line, 1, open_paren - 1)
    params_text = substr(line, open_paren + 1, close_paren - open_paren - 1)
    rest = substr(line, close_paren + 1)
    sub(/^[ \t]*REDDE[ \t]+/, "", rest)
    sub(/;[ \t]*$/, "", rest)
    ret_type = map_type(rest)

    ordinal += 1
    wrapper_name = "cct_boot_rit_" name "_" ordinal
    target_name = "cct_boot_rit_" map_target_name(name)

    c_params = ""
    call_args = ""
    params_text = trim(params_text)
    if (params_text == "") {
        c_params = "void"
    } else {
        n = split(params_text, parts, ",")
        for (i = 1; i <= n; i++) {
            part = trim(parts[i])
            m = split(part, words, /[ \t]+/)
            param_name = words[m]
            param_type = ""
            for (j = 1; j < m; j++) {
                if (param_type != "") param_type = param_type " "
                param_type = param_type words[j]
            }
            if (c_params != "") c_params = c_params ", "
            c_params = c_params map_type(param_type) " " param_name
            if (call_args != "") call_args = call_args ", "
            call_args = call_args param_name
        }
    }

    print ret_type " " wrapper_name "(" c_params ") {"
    if (ret_type == "void") {
        print "    " target_name "(" call_args ");"
    } else {
        print "    return " target_name "(" call_args ");"
    }
    print "}"
    print ""
}
