import io.github.treesitter.jtreesitter.Language;
import io.github.treesitter.jtreesitter.cooklang.TreeSitterCooklang;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;

public class TreeSitterCooklangTest {
    @Test
    public void testCanLoadLanguage() {
        assertDoesNotThrow(() -> new Language(TreeSitterCooklang.language()));
    }
}
