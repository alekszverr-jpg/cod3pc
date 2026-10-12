// Recover only exact two-instruction this-adjustment tail thunks referenced
// from .rdata and branching to an already registered game function.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.mem.Memory;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.TreeSet;
import java.util.HashSet;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class ExportCod3Thunks extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new Exception("Expected mapping .cpp and output TOML paths");
        HashSet<Long> registered = new HashSet<>();
        Matcher matcher = Pattern.compile("\\{ 0x([0-9A-Fa-f]{8}),").matcher(Files.readString(Path.of(args[0])));
        while (matcher.find()) registered.add(Long.parseLong(matcher.group(1), 16));
        Memory memory = currentProgram.getMemory();
        MemoryBlock rdata = memory.getBlock(".rdata");
        if (rdata == null) throw new Exception("Expected .rdata");
        TreeSet<Long> found = new TreeSet<>();
        StringBuilder out = new StringBuilder("# Ghidra-verified .rdata-referenced this-adjustment thunks.\n# For default.xex SHA-256 2944eec7d1231ad6798b5f9f8adf8855f5e489296b22eab45b27a577cee23692\n[functions]\n");
        // Retain earlier reviewed entries when a later mapping already contains them.
        Path destination = Path.of(args[1]);
        if (Files.exists(destination)) {
            for (String line : Files.readAllLines(destination)) {
                if (line.matches("\"0x[0-9A-Fa-f]{8}\" = \\{ size = 8 \\} # target 0x[0-9A-Fa-f]{8}, rdata [0-9A-Fa-f]+")) {
                    out.append(line).append("\n");
                    found.add(Long.parseLong(line.substring(3, 11), 16));
                }
            }
        }
        for (Address p = rdata.getStart(); p.compareTo(rdata.getEnd().subtract(3)) <= 0; p = p.add(4)) {
            monitor.checkCancelled();
            long ptr = Integer.toUnsignedLong(memory.getInt(p));
            if ((ptr & 3) != 0 || registered.contains(ptr) || found.contains(ptr)) continue;
            Address a = toAddr(ptr);
            MemoryBlock block = memory.getBlock(a);
            if (block == null || !block.isExecute() || !block.contains(a.add(7))) continue;
            int adjust = memory.getInt(a);
            int branch = memory.getInt(a.add(4));
            if ((adjust & 0xFFFF0000) != 0x38630000 || (branch & 0xFC000003) != 0x48000000) continue;
            int displacement = (branch & 0x03FFFFFC);
            if ((displacement & 0x02000000) != 0) displacement |= 0xFC000000;
            long target = (ptr + 4 + displacement) & 0xFFFFFFFFL;
            if (!registered.contains(target)) continue;
            disassemble(a);
            if (getInstructionAt(a) == null || getInstructionAt(a.add(4)) == null) continue;
            found.add(ptr);
            println(String.format("THUNK %08X rdata=%s words=%08X,%08X target=%08X %s; %s", ptr, p, adjust, branch, target, getInstructionAt(a), getInstructionAt(a.add(4))));
            out.append(String.format("\"0x%08X\" = { size = 8 } # target 0x%08X, rdata %s\n", ptr, target, p));
        }
        Files.writeString(destination, out.toString());
        println("EXPORTED " + found.size() + " thunks to " + args[1]);
    }
}
