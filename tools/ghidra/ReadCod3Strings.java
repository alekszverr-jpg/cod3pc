// Read-only local evidence; output containing game data stays in private logs.
// @category COD3
import ghidra.app.script.GhidraScript;

public class ReadCod3Strings extends GhidraScript {
    public void run() throws Exception {
        for (String argument : getScriptArgs()) {
            var address = toAddr(Long.decode(argument));
            StringBuilder text = new StringBuilder();
            for (int i = 0; i < 512; ++i) {
                int value = currentProgram.getMemory().getByte(address.add(i)) & 255;
                if (value == 0) break;
                if (value >= 32 && value <= 126) text.append((char)value);
                else text.append(String.format("\\x%02X", value));
            }
            println("STRING " + address + " " + text);
        }
    }
}
