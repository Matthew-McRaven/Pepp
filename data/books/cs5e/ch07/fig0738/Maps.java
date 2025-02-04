package fig0738;

import java.util.EnumMap;
import java.util.HashMap;
import java.util.Map;

/**
 * The maps connecting string values with enumerated values.
 *
 * <p>
 * File: <code>Maps.java</code>
 *
 * @author J. Stanley Warford
 */
public final class Maps {

   public static final Map<String, Mnemon> unaryMnemonTable;
   public static final Map<String, Mnemon> nonUnaryMnemonTable;
   public static final Map<Mnemon, String> mnemonStringTable;

   static {
      unaryMnemonTable = new HashMap<>();
      unaryMnemonTable.put("stop", Mnemon.M_STOP);
      unaryMnemonTable.put("end", Mnemon.M_END);

      nonUnaryMnemonTable = new HashMap<>();
      nonUnaryMnemonTable.put("neg", Mnemon.M_NEG);
      nonUnaryMnemonTable.put("abs", Mnemon.M_ABS);
      nonUnaryMnemonTable.put("add", Mnemon.M_ADD);
      nonUnaryMnemonTable.put("sub", Mnemon.M_SUB);
      nonUnaryMnemonTable.put("mul", Mnemon.M_MUL);
      nonUnaryMnemonTable.put("div", Mnemon.M_DIV);
      nonUnaryMnemonTable.put("set", Mnemon.M_SET);

      mnemonStringTable = new EnumMap<>(Mnemon.class);
      mnemonStringTable.put(Mnemon.M_NEG, "neg");
      mnemonStringTable.put(Mnemon.M_ABS, "abs");
      mnemonStringTable.put(Mnemon.M_ADD, "add");
      mnemonStringTable.put(Mnemon.M_SUB, "sub");
      mnemonStringTable.put(Mnemon.M_MUL, "mul");
      mnemonStringTable.put(Mnemon.M_DIV, "div");
      mnemonStringTable.put(Mnemon.M_SET, "set");
      mnemonStringTable.put(Mnemon.M_STOP, "stop");
      mnemonStringTable.put(Mnemon.M_END, "end");
   }
}
