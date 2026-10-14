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
        String[] args = getScriptArgs();
        if (args.length != 2 || !args[1].matches("[0-9A-Fa-f]{64}"))
            throw new Exception("Expected output TOML and verified module SHA256");
        Memory memory = currentProgram.getMemory();
        MemoryBlock text = memory.getBlock(".text");
        ArrayList<Address> starts = new ArrayList<>();
        for (Address a = text.getStart(); a.compareTo(text.getEnd().subtract(19)) < 0; a = a.add(4)) {
            monitor.checkCancelled();
            if ((memory.getInt(a) & 0xFFFF0000) == 0x3D600000 &&
                    (memory.getInt(a.add(4)) & 0xFFFF0000) == 0x396B0000
                    && memory.getInt(a.add(8)) == 0x7C2A0B78
                    && memory.getInt(a.add(12)) == 0x914B0000
                    && memory.getInt(a.add(16)) == 0x3821FE70
                    && memory.getInt(a.add(20)) == 0xF8610008) starts.add(a);
        }
        StringBuilder out = new StringBuilder("# " + currentProgram.getName() + " SHA256 " + args[1].toLowerCase() +
                "\n# Ghidra: PPC context-save prologue and final interface wait call.\n");
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
            for (int offset=0;offset<24;offset+=4)
                println("PROLOGUE " + start.add(offset) + " " + String.format("%08X",memory.getInt(start.add(offset))));
            out.append(String.format("\n[[midasm_hook]]\naddress = 0x%08X\nname = \"Cod3ReturnFromGuestYield\"\nafter_instruction = true\nreturn = true\n", wait.getOffset()));
            count++;
        }
        if (count == 0) throw new Exception("No exact wrappers found");
        Files.writeString(Path.of(args[0]), out.toString());
        println("EXPORTED " + count);
    }
}
