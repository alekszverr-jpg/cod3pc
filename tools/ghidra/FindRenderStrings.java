// Locate original render-setting strings and existing references; no patches.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

public class FindRenderStrings extends GhidraScript {
    private int word(byte[] data, int offset) {
        return ((data[offset]&255)<<24)|((data[offset+1]&255)<<16)|
               ((data[offset+2]&255)<<8)|(data[offset+3]&255);
    }
    public void run() throws Exception {
        String[] queries = getScriptArgs();
        MemoryBlock code=currentProgram.getMemory().getBlock(".text");
        byte[] instructions=new byte[(int)code.getSize()];
        currentProgram.getMemory().getBytes(code.getStart(),instructions);
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.getName().equals(".rdata")) continue;
            byte[] bytes = new byte[(int)block.getSize()];
            currentProgram.getMemory().getBytes(block.getStart(), bytes);
            int start = 0;
            for (int i = 0; i < bytes.length; ++i) {
                if (bytes[i] != 0) continue;
                if (i - start >= 3 && i - start < 300) {
                    String s = new String(bytes, start, i-start, StandardCharsets.ISO_8859_1);
                    String lower = s.toLowerCase(Locale.ROOT);
                    boolean matches = lower.contains("focus") || lower.contains("depth of") ||
                            (lower.contains("dof") && !lower.contains("offset") && !lower.contains("ofs"));
                    if (queries.length > 0) {
                        matches = false;
                        for (String query : queries) if (lower.contains(query.toLowerCase(Locale.ROOT))) matches = true;
                    }
                    if (matches) {
                        boolean ascii = true;
                        for (int j=start; j<i; ++j) if (bytes[j]<32 || bytes[j]>126) ascii=false;
                        if (ascii) {
                            var address=block.getStart().add(start);
                            println("RENDER_STRING " + address + " " + s);
                            ReferenceIterator refs=currentProgram.getReferenceManager().getReferencesTo(address);
                            while(refs.hasNext()) {
                                Reference ref=refs.next();
                                println("REF " + ref.getFromAddress() + " " + ref.getReferenceType());
                            }
                            long av=address.getOffset();
                            for(int off=0;off+3<instructions.length;off+=4) {
                                int ins=word(instructions,off),op=ins>>>26;
                                if((op!=14 && op!=24) || (ins&65535)!=(av&65535)) continue;
                                int base=(ins>>>16)&31;
                                if(op==24) base=(ins>>>21)&31;
                                int high=(int)((av+(op==14?0x8000:0))>>>16)&65535;
                                for(int prior=Math.max(0,off-64);prior<off;prior+=4) {
                                    int upper=word(instructions,prior);
                                    if((upper>>>26)==15 && ((upper>>>16)&31)==0 &&
                                            ((upper>>>21)&31)==base && (upper&65535)==high) {
                                        println("TEXT_REF_CANDIDATE " + code.getStart().add(prior) + " " +
                                                String.format("%08X",upper) + " / " +
                                                code.getStart().add(off) + " " + String.format("%08X",ins));
                                    }
                                }
                            }
                            byte[] pointer=new byte[]{(byte)(av>>24),(byte)(av>>16),(byte)(av>>8),(byte)av};
                            for(MemoryBlock search : currentProgram.getMemory().getBlocks()) {
                                if(search.isExecute()) continue;
                                var at=search.getStart();
                                while(at.compareTo(search.getEnd())<0) {
                                    var hit=currentProgram.getMemory().findBytes(at,search.getEnd(),pointer,null,true,monitor);
                                    if(hit==null) break;
                                    println("POINTER_REF " + hit);
                                    for(int j=-2;j<=4;j++) {
                                        var word=hit.add(j*4);
                                        if(search.contains(word) && search.contains(word.add(3)))
                                            println("PTR_CONTEXT " + word + " " + String.format("%08X", currentProgram.getMemory().getInt(word)));
                                    }
                                    at=hit.add(4);
                                }
                            }
                        }
                    }
                }
                start = i+1;
            }
        }
    }
}
