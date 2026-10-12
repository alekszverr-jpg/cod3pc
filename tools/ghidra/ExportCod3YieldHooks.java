// Exact context-save wrappers, followed by a tail call to a script wait service.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;

public class ExportCod3YieldHooks extends GhidraScript {
    public void run() throws Exception {
        Memory memory = currentProgram.getMemory();
        MemoryBlock text = memory.getBlock(".text");
        ArrayList<Address> starts = new ArrayList<>();
        for (Address a = text.getStart(); a.compareTo(text.getEnd().subtract(19)) < 0; a = a.add(4)) {
            monitor.checkCancelled();
            if (memory.getInt(a) == 0x3D608925 && memory.getInt(a.add(4)) == 0x396B52C4
                    && memory.getInt(a.add(16)) == 0x3821FE70) starts.add(a);
        }
        StringBuilder out = new StringBuilder("# Saint-Lo SHA256 49e38a116b8f3c5e81c5206fd461496a6da518cf9427843b38f2cad33910ef2b\n# Ghidra: exact PPC context-save prologue and final interface wait call.\n");
        int count = 0;
        for (Address start : starts) {
            Address wait = null;
            for (int offset = 0x100; offset < 0x180; offset += 4) {
                Address a = start.add(offset);
                if (!text.contains(a.add(8))) break;
                int load = memory.getInt(a);
                int slot = load & 0xFFFF;
                if ((load & 0xFFFF0000) == 0x816B0000 && (slot == 76 || slot == 80 || slot == 84)
                        && memory.getInt(a.add(4)) == 0x7D6903A6 && memory.getInt(a.add(8)) == 0x4E800421) {
                    wait = a.add(8);
                    break;
                }
            }
            if (wait == null) continue;
            disassemble(start);
            disassemble(wait);
            if (getInstructionAt(wait) == null) throw new Exception("Wait did not disassemble");
            println("YIELD_WRAPPER " + start + " wait=" + wait + " " + getInstructionAt(wait));
            out.append(String.format("\n[[midasm_hook]]\naddress = 0x%08X\nname = \"Cod3ReturnFromGuestYield\"\nafter_instruction = true\nreturn = true\n", wait.getOffset()));
            count++;
        }
        if (count == 0) throw new Exception("No exact wrappers found");
        Files.writeString(Path.of(getScriptArgs()[0]), out.toString());
        println("EXPORTED " + count);
    }
}
