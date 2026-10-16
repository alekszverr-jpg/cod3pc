// Export SDK-independent, XEXLoader-loaded sections for local byte comparison.
// @category COD3
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.MemoryBlock;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;

public class ExportCod3Image extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected private output directory");
        Path output = Path.of(args[0]);
        Files.createDirectories(output);
        StringBuilder manifest = new StringBuilder();
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized()) continue;
            byte[] data = new byte[Math.toIntExact(block.getSize())];
            currentProgram.getMemory().getBytes(block.getStart(), data);
            String file = block.getStart().toString() + ".bin";
            Files.write(output.resolve(file), data);
            String hash = java.util.HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(data));
            manifest.append(block.getName()).append('\t').append(block.getStart()).append('\t')
                .append(data.length).append('\t').append(block.isExecute()).append('\t').append(hash).append('\n');
        }
        Files.writeString(output.resolve("sections.tsv"), manifest.toString());
        println("Exported loaded sections to " + output);
    }
}
