package io.cucumber.query;

import io.cucumber.messages.types.Timestamp;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class TimestampComparatorTest {
    
    private final TimestampComparator comparator = new TimestampComparator();
    
    @Test
    void identity(){
        Timestamp a = new Timestamp(1L, 1L);
        Timestamp b = new Timestamp(1L, 1L);

        assertThat(comparator.compare(a, b)).isEqualTo(0);
        assertThat(comparator.compare(b, a)).isEqualTo(0);
    }
    
    @Test
    void onSeconds(){
        Timestamp a = new Timestamp(1L, 1L);
        Timestamp b = new Timestamp(2L, 2L);
        assertThat(comparator.compare(a, b)).isEqualTo(-1);
        assertThat(comparator.compare(b, a)).isEqualTo(1);
    }
    
    @Test
    void onNanoSeconds(){
        Timestamp a = new Timestamp(1L, 1L);
        Timestamp b1 = new Timestamp(1L, 0L);
        Timestamp b2 = new Timestamp(1L, 2L);
        
        assertThat(comparator.compare(a, b1)).isEqualTo(1);
        assertThat(comparator.compare(b1, a)).isEqualTo(-1);
        
        assertThat(comparator.compare(a, b2)).isEqualTo(-1);
        assertThat(comparator.compare(b2, a)).isEqualTo(1);
        
    }
}
