
import "std"

#[nomangle]
fun main() {

    ref u8: buf = std::mem::alloc(64 as u32)
    ref u8: adr = buf

    std::mem::realloc(buf, 32 as u32)

    std::io::print("Hello, World\n", 13 as u32)

    std::mem::free(buf)
}
