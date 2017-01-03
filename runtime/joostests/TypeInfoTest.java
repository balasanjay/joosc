package joostests;

import static org.junit.Assert.assertEquals;
import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import __joos_internal__.TypeInfo;

@RunWith(JUnit4.class)
public class TypeInfoTest {
  @Test
  public void test1() {
    /*
     *      3
     *      |
     *    4 2
     *     \|
     *      1
     *      |
     *      0
     */
    TypeInfo.num_types = 5;
    TypeInfo[] types = new TypeInfo[TypeInfo.num_types];
    // 3.
    {
      TypeInfo[] parents = new TypeInfo[0];
      types[3] = new TypeInfo(3, parents);
    }
    // 4.
    {
      TypeInfo[] parents = new TypeInfo[0];
      types[4] = new TypeInfo(4, parents);
    }
    // 2.
    {
      TypeInfo[] parents = new TypeInfo[1];
      parents[0] = types[3];
      types[2] = new TypeInfo(2, parents);
    }
    // 1.
    {
      TypeInfo[] parents = new TypeInfo[2];
      parents[0] = types[2];
      parents[1] = types[4];
      types[1] = new TypeInfo(1, parents);
    }
    // 0.
    {
      TypeInfo[] parents = new TypeInfo[1];
      parents[0] = types[1];
      types[0] = new TypeInfo(0, parents);
    }

    assertThat(TypeInfo.InstanceOf(types[0], types[1])).isTrue();
    assertThat(TypeInfo.InstanceOf(types[0], types[3])).isTrue();
    assertThat(TypeInfo.InstanceOf(types[0], types[4])).isTrue();
    assertThat(TypeInfo.InstanceOf(types[1], types[4])).isTrue();
    assertThat(TypeInfo.InstanceOf(types[0], types[0])).isTrue();
    assertThat(TypeInfo.InstanceOf(types[2], types[4])).isFalse();
    assertThat(TypeInfo.InstanceOf(types[2], types[0])).isFalse();
    assertThat(TypeInfo.InstanceOf(types[4], types[3])).isFalse();
  }
}
