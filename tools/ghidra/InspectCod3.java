// Local Ghidra evidence for the two unresolved direct branch targets.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;

public class InspectCod3 extends GhidraScript {
    public void run() throws Exception {
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            println("SECTION " + b.getName() + " " + b.getStart() + " " + b.getEnd() + " execute=" + b.isExecute());
        }
        for (long value : new long[] {0x822D0498L, 0x822D2140L, 0x822D1308L, 0x822D24E8L, 0x822C27F8L, 0x822CAB78L}) {
            Address a = toAddr(value);
            Function f = getFunctionContaining(a);
            println("TARGET " + a + " function=" + (f == null ? "none" : f.getName() + " " + f.getBody()));
            disassemble(a);
            for (int i = -4; i < 24; i++) {
                Address p = a.add(i * 4);
                Instruction ins = getInstructionAt(p);
                println(p + " bytes=" + String.format("%08X", currentProgram.getMemory().getInt(p)) + " " + ins);
            }
        }
    }
}
