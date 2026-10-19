// Read-only candidate lis + D-form address references; confirm data flow manually.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

public class FindPpcAddressReferences extends GhidraScript {
    public void run() throws Exception {
        var code = currentProgram.getMemory().getBlock(".text");
        for (String arg : getScriptArgs()) {
            long target = Long.decode(arg) & 0xffffffffL;
            println(String.format("ADDRESS_TARGET %08X", target));
            for (var at = code.getStart(); at.compareTo(code.getEnd().subtract(259)) <= 0; at = at.add(4)) {
                monitor.checkCancelled();
                int hi = currentProgram.getMemory().getInt(at);
                if ((hi >>> 26) != 15 || ((hi >>> 16) & 31) != 0) continue;
                int register = (hi >>> 21) & 31;
                long base = ((long)(short)(hi & 65535) << 16) & 0xffffffffL;
                for (int n = 1; n <= 64; n++) {
                    var use = at.add(n * 4);
                    int lo = currentProgram.getMemory().getInt(use);
                    int opcode = lo >>> 26;
                    if (opcode != 14 && opcode != 32 && opcode != 36 && opcode != 48 && opcode != 52) continue;
                    if (((lo >>> 16) & 31) != register) continue;
                    if (((base + (short)(lo & 65535)) & 0xffffffffL) != target) continue;
                    Function f = getFunctionContaining(use);
                    println("ADDRESS_CANDIDATE " + at + " use=" + use + " function=" +
                        (f == null ? "none" : f.getEntryPoint().toString()));
                }
            }
        }
    }
}
