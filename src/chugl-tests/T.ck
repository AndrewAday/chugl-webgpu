// Test Harness
public class T {

    "" => string context;

    fun static void println(string s) {
        chout <= s <= IO.nl();
    }
    
    fun static void err(string s) {
        cherr <= s <= IO.nl();
    }

    fun static void assert(int bool) {
        if (!bool) err("Assertion failed");
    }

    fun static void assert(int bool, string s) {
        if (!bool) err("Assertion failed: " + s);
    }

    fun void assert(string code) {
        Machine.eval(
            this.context + 
            " T.assert(" + code + ", \"" + code + "\");
        ");
    }
}