package org.zyre;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class Utils {
	
	public static HashMap<String,String> parseMsg(String msg) {
		HashMap<String,String> result = new HashMap<String,String>();
		List<String> pairs = Arrays.asList( msg.split(("\\|")) );

		if (pairs.size() != 4) {
			System.err.println("recv() did not return exactly 4 key/value pairs");
			return result;
		}

		for (String pair : pairs) {
			List<String> kv = Arrays.asList( pair.split(("::")) );
			if (kv.size() == 0) {
				// key and value are empty - do nothing
			}
			else if (kv.size() == 1) {
				// value is null
				result.put(kv.get(0), null);
			}
			else {
				result.put(kv.get(0), kv.get(1));
			}
		}
		
		return result;
	}

}
