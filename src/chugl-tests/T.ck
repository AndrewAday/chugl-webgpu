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

    fun static int strEquals(string a, string b) {
        // chuck can't compare a null string to a non-null string 
        if (a == null && b == null) {
            return 1;
        }

        false => int equals;
        if (a == null || b == null) {
            false => equals;
        } else {
            (a == b) => equals;
        }

        if (!equals) {
            // because chuck can't concatenate null strings
            err("Strings not equal:");
            <<< a >>>;
            err("!=");
            <<< b >>>;
        }

        return equals;
    }

    fun static int arrayEquals(int a[], int b[]) {
        if (a == null && b == null) {
            return 1;
        }
        if (a == null || b == null) {
            return 0;
        }
        if (a.size() != b.size()) {
            return 0;
        }
        for (0 => int i; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                return 0;
            }
        }
        return 1;
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
            if (num_components > 1) {
                chout <= "( ";
            }
            for (i => int j; j < i + num_components; ++j) {
                chout <= arr[j] <= ", ";
            }
            if (num_components > 1) {
                chout <= ") ";
            }
        }
        chout <= "]" <= IO.nl();
    }
    

    fun static void printArray(int arr[]) {
        T.printArray(arr, 1);
    }
}