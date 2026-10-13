// Read-only raw address references, including PPC branches and pointer tables.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.MemoryBlock;

public class FindAddressReferences extends GhidraScript {
    public void run() throws Exception {
        for (String argument : getScriptArgs()) {
            long target = Long.decode(argument) & 0xFFFFFFFFL;
            println(String.format("REFERENCE_TARGET %08X", target));
            for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
                for (var at=block.getStart(); at.compareTo(block.getEnd().subtract(3))<=0; at=at.add(4)) {
                    monitor.checkCancelled();
                    int word=currentProgram.getMemory().getInt(at);
                    if (Integer.toUnsignedLong(word)==target) {
                        println("POINTER " + at + " block=" + block.getName());
                        var start=at;
                        while (!block.isExecute() && block.contains(start.subtract(4))) {
                            long value=Integer.toUnsignedLong(currentProgram.getMemory().getInt(start.subtract(4)));
                            var pointed=currentProgram.getMemory().getBlock(toAddr(value));
                            if (pointed==null || !pointed.isExecute()) break;
                            start=start.subtract(4);
                        }
                        println("POINTER_RUN_START " + start + " offset=" + at.subtract(start));
                        if (!block.isExecute()) for (int n=-8;n<=8;n++) {
                            var item=at.add(4*n);
                            if (block.contains(item) && block.contains(item.add(3)))
                                println("CONTEXT " + item + " " + String.format("%08X",currentProgram.getMemory().getInt(item)));
                        }
                    }
                    if (!block.isExecute() || (word>>>26)!=18 || (word&2)!=0) continue;
                    int delta=word&0x03FFFFFC;
                    if ((delta&0x02000000)!=0) delta|=0xFC000000;
                    if (((at.getOffset()+delta)&0xFFFFFFFFL)==target)
                        println("BRANCH " + at + " link=" + ((word&1)!=0));
                }
            }
        }
    }
}
