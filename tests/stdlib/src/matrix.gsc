
import "std"

fun ulog10(in u32: num, out u32: log)
{
    u32: base = 1 as u32
    u32: count = 0 as u32

    while base < num
    {
        base = base * 10 as u32
        count = count + 1 as u32
    }

    if count == 0 as u32 {
        count = 1 as u32
    }

    log = count
}

fun u32ToCstr(in u32: number)(out cstr: result, out u32: len)
{
    u32: bytes = 0 as u32
    ulog10(number, bytes)

    cstr: buf = 0 as cstr
    heapAlloc(bytes)(buf as ref u8)

    u32: idx = bytes - 1 as u32
    u32: tmp = number
    while (idx > 0 || idx == 0)
    {
        u32: digit = 0 as u32
        mod(tmp, 10 as u32, digit)

        buf[idx] = (digit + '0') as u8

        tmp = tmp / 10 as u32
        idx = idx - 1 as u32
    }

    len = bytes
    result = buf
}

fun printU32(in u32: val)
{
    cstr: str = 0 as cstr
    u32: len = 0 as u32

    u32ToCstr(val)(str, len)

    handle: stdout = nullHandle
    getStdoutHandle(stdout)

    u32: written = 0 as u32
    writeBytes(stdout, str, len, written)

    heapFree(str)

    writeBytes(stdout, " ", 1 as u32, written)
}

fun test_matrix()
{
    ref ref u32: matrix
    heapAlloc((8 * 4) as u32)(matrix as ref u8)

    u32: written = 0 as u32
    handle: stdout = nullHandle
    getStdoutHandle(stdout)

    u32: idx = 0 as u32
    while idx < 4 {
        heapAlloc((4 * 4) as u32)(matrix[idx] as ref u8)

        u32: idy = 0 as u32
        while idy < 4 {
            matrix[idx][idy] = idy

            printU32(matrix[idx][idy])

            idy = idy + 1 as u32
        }
        writeBytes(stdout, "\n", 1 as u32, written)

        heapFree(matrix[idx] as ref u8)
        idx = idx + 1 as u32
    }

    heapFree(matrix as ref u8)
}

fun main(): u32
{
    test_matrix()
    exit_code = 0

    ret 0
}
