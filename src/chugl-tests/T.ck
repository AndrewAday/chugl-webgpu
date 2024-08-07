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

    fun static int feq(float a, float b) {
        return Math.fabs(a - b) < 0.0001;
    }

    fun void assert(string code) {
        Machine.eval(
            this.context + 
            " T.assert(" + code + ", \"" + code + "\"); "
        );
    }

    fun static void printArray(float arr[], int num_components) {
        chout <= "[ ";
        for (0 => int i; i < arr.size(); num_components +=> i) {
            chout <= "( ";
            for (i => int j; j < i + num_components; ++j) {
                chout <= arr[j] <= ", ";
            }
            chout <= ") ";
        }
        chout <= " ]" <= IO.nl();
    }

    fun static void printArray(int arr[], int num_components) {
        chout <= "[ ";
        for (0 => int i; i < arr.size(); num_components +=> i) {
            chout <= "( ";
            for (i => int j; j < i + num_components; ++j) {
                chout <= arr[j] <= ", ";
            }
            chout <= ") ";
        }
        chout <= " ]" <= IO.nl();
    }
}