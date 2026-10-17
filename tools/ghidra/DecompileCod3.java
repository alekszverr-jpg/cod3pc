// Local diagnostic output only; keep derived game code in ignored logs.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.program.model.listing.Function;

public class DecompileCod3 extends GhidraScript {
    public void run() throws Exception {
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram))
                throw new IllegalStateException("Cannot open program for decompilation");
            for (String arg : getScriptArgs()) {
                var address = toAddr(Long.decode(arg));
                Function function = getFunctionContaining(address);
                println("DECOMPILE_TARGET " + address + " function=" +
                        (function == null ? "none" : function.getEntryPoint()));
                if (function == null) continue;
                var result = decompiler.decompileFunction(function, 60, monitor);
                if (!result.decompileCompleted()) {
                    println("DECOMPILE_ERROR " + result.getErrorMessage());
                    continue;
                }
                println(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
    }
}
