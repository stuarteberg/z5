package zarr_pp.n5;

import org.janelia.saalfeldlab.n5.N5Writer;
import org.janelia.saalfeldlab.n5.DataType;
import org.janelia.saalfeldlab.n5.CompressionType;

import org.janelia.saalfeldlab.n5.ByteArrayDataBlock;
import org.janelia.saalfeldlab.n5.ShortArrayDataBlock;
import org.janelia.saalfeldlab.n5.IntArrayDataBlock;
import org.janelia.saalfeldlab.n5.LongArrayDataBlock;
import org.janelia.saalfeldlab.n5.FloatArrayDataBlock;
import org.janelia.saalfeldlab.n5.DoubleArrayDataBlock;

import org.janelia.saalfeldlab.n5.DatasetAttributes;
import org.janelia.saalfeldlab.n5.N5;

import java.io.IOException;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.util.Random;

public class App {

    // TODO irregular shapes
    static private long[] dimensions = new long[]{111, 121, 113};
    static private int[] blockSize = new int[]{17, 25, 14};
	static private String testDir = "../n5_test_data";
    static private String emptySet = "array_fv.n5";
    
    static private String byteSetRaw = "array_byte_raw.n5";
    static private String shortSetRaw = "array_short_raw.n5";
    static private String intSetRaw = "array_int_raw.n5";
    static private String longSetRaw = "array_long_raw.n5";
    static private String floatSetRaw = "array_float_raw.n5";
    static private String doubleSetRaw = "array_double_raw.n5";
    
    static private String byteSetGzip = "array_byte_gzip.n5";
    static private String shortSetGzip = "array_short_gzip.n5";
    static private String intSetGzip = "array_int_gzip.n5";
    static private String longSetGzip = "array_long_gzip.n5";
    static private String floatSetGzip = "array_float_gzip.n5";
    static private String doubleSetGzip = "array_double_gzip.n5";
        
    static byte[] byteBlock;
    static short[] shortBlock;
    static int[] intBlock;
    static long[] longBlock;
    static float[] floatBlock;
    static double[] doubleBlock;

    static private N5Writer n5;
    
    public static void makeTestDataRaw() throws IOException {
		
		n5 = N5.openFSWriter(testDir);
        byteBlock = new byte[blockSize[0] * blockSize[1] * blockSize[2]];
        shortBlock = new short[blockSize[0] * blockSize[1] * blockSize[2]];
        intBlock = new int[blockSize[0] * blockSize[1] * blockSize[2]];
        longBlock = new long[blockSize[0] * blockSize[1] * blockSize[2]];
        floatBlock = new float[blockSize[0] * blockSize[1] * blockSize[2]];
        doubleBlock = new double[blockSize[0] * blockSize[1] * blockSize[2]];

        // random test data
        final Random rnd = new Random();
        for(int i = 0; i < doubleBlock.length; i++) {
            byteBlock[i]  =42;
            shortBlock[i] =42;
            intBlock[i]  = 42;
            longBlock[i] = 42;
            floatBlock[i]  = 42;
            doubleBlock[i] = 42;
            // TODO would be better to create random values, but it's a pain to get
            // them to c++ consistently
            //byteBlock[i] = (byte) rnd.nextInt();
            //shortBlock[i] = (short) rnd.nextInt();
            //intBlock[i] = rnd.nextInt();
            //longBlock[i] = rnd.nextLong();
            //floatBlock[i] = Float.intBitsToFloat(rnd.nextInt());
            //doubleBlock[i] = Double.longBitsToDouble(rnd.nextLong());
        }
        
        //// write it to a save to read format
        //// byte
        //final String byteName = "../n5_test_data/byte_data.in";
        //BufferedWriter byteWriter = null;
        //byteWriter = new BufferedWriter(new FileWriter(byteName));
        //for(int i = 0; i < byteBlock.length; i++) {
        //    byteWriter.write(Byte.toString(byteBlock[i]));
        //    byteWriter.newLine();
        //}
        //// short
        //final String shortName = "../n5_test_data/short_data.in";
        //BufferedWriter shortWriter = null;
        //shortWriter = new BufferedWriter(new FileWriter(shortName));
        //for(int i = 0; i < shortBlock.length; i++) {
        //    shortWriter.write(Short.toString(shortBlock[i]));
        //    shortWriter.newLine();
        //}
        //// int
        //final String intName = "../n5_test_data/int_data.in";
        //BufferedWriter intWriter = null;
        //intWriter = new BufferedWriter(new FileWriter(intName));
        //for(int i = 0; i < intBlock.length; i++) {
        //    intWriter.write(Integer.toString(intBlock[i]));
        //    intWriter.newLine();
        //}
        //// long
        //final String longName = "../n5_test_data/long_data.in";
        //BufferedWriter longWriter = null;
        //longWriter = new BufferedWriter(new FileWriter(longName));
        //for(int i = 0; i < longBlock.length; i++) {
        //    longWriter.write(Long.toString(longBlock[i]));
        //    longWriter.newLine();
        //}
        //// float
        //final String floatName = "../n5_test_data/float_data.in";
        //BufferedWriter floatWriter = null;
        //floatWriter = new BufferedWriter(new FileWriter(floatName));
        //for(int i = 0; i < floatBlock.length; i++) {
        //    floatWriter.write(Float.toString(floatBlock[i]));
        //    floatWriter.newLine();
        //}
        //// double
        //final String doubleName = "double_data.in";
        //BufferedWriter doubleWriter = null;
        //doubleWriter = new BufferedWriter(new FileWriter(doubleName));
        //for(int i = 0; i < doubleBlock.length; i++) {
        //    doubleWriter.write(Double.toString(doubleBlock[i]));
        //    doubleWriter.newLine();
        //}


        // byte set
        n5.createDataset(byteSetRaw, dimensions, blockSize, DataType.INT8, CompressionType.RAW);
        final DatasetAttributes attrsByte = n5.getDatasetAttributes(byteSetRaw);
        // short set
        n5.createDataset(shortSetRaw, dimensions, blockSize, DataType.INT16, CompressionType.RAW);
        final DatasetAttributes attrsShort = n5.getDatasetAttributes(shortSetRaw);
        // int set
        n5.createDataset(intSetRaw, dimensions, blockSize, DataType.INT32, CompressionType.RAW);
        final DatasetAttributes attrsInt = n5.getDatasetAttributes(intSetRaw);
        // long set
        n5.createDataset(longSetRaw, dimensions, blockSize, DataType.INT64, CompressionType.RAW);
        final DatasetAttributes attrsLong = n5.getDatasetAttributes(longSetRaw);
        // float set
        n5.createDataset(floatSetRaw, dimensions, blockSize, DataType.FLOAT32, CompressionType.RAW);
        final DatasetAttributes attrsFloat = n5.getDatasetAttributes(floatSetRaw);
        // double set
        n5.createDataset(doubleSetRaw, dimensions, blockSize, DataType.FLOAT64, CompressionType.RAW);
        final DatasetAttributes attrsDouble = n5.getDatasetAttributes(doubleSetRaw);

        for(int x = 0; x < blockSize[0]; x++) {
            for(int y = 0; y < blockSize[1]; y++) {
                for(int z = 0; z < blockSize[2]; z++) {
                    
                    // write byte
                    final ByteArrayDataBlock byteDataBlock = new ByteArrayDataBlock(
                        blockSize, new long[]{x, y, z}, byteBlock
                    );
                    n5.writeBlock(byteSetRaw, attrsByte, byteDataBlock);
                    
                    // write short
                    final ShortArrayDataBlock shortDataBlock = new ShortArrayDataBlock(
                        blockSize, new long[]{x, y, z}, shortBlock
                    );
                    n5.writeBlock(shortSetRaw, attrsShort, shortDataBlock);
                    
                    // write int
                    final IntArrayDataBlock intDataBlock = new IntArrayDataBlock(
                        blockSize, new long[]{x, y, z}, intBlock
                    );
                    n5.writeBlock(intSetRaw, attrsInt, intDataBlock);
            
                    // write long
                    final LongArrayDataBlock longDataBlock = new LongArrayDataBlock(
                        blockSize, new long[]{x, y, z}, longBlock
                    );
                    n5.writeBlock(longSetRaw, attrsLong, longDataBlock);
                    
                    // write float
                    final FloatArrayDataBlock floatDataBlock = new FloatArrayDataBlock(
                        blockSize, new long[]{x, y, z}, floatBlock
                    );
                    n5.writeBlock(floatSetRaw, attrsFloat, floatDataBlock);
                    
                    // write double
                    final DoubleArrayDataBlock doubleDataBlock = new DoubleArrayDataBlock(
                        blockSize, new long[]{x, y, z}, doubleBlock
                    );
                    n5.writeBlock(doubleSetRaw, attrsDouble, doubleDataBlock);
                }
            }
        }
    }
    
    
    public static void makeTestDataGzip() throws IOException {
		
		n5 = N5.openFSWriter(testDir);
        byteBlock = new byte[blockSize[0] * blockSize[1] * blockSize[2]];
        shortBlock = new short[blockSize[0] * blockSize[1] * blockSize[2]];
        intBlock = new int[blockSize[0] * blockSize[1] * blockSize[2]];
        longBlock = new long[blockSize[0] * blockSize[1] * blockSize[2]];
        floatBlock = new float[blockSize[0] * blockSize[1] * blockSize[2]];
        doubleBlock = new double[blockSize[0] * blockSize[1] * blockSize[2]];

        for(int i = 0; i < doubleBlock.length; i++) {
            byteBlock[i] = 42;
            shortBlock[i] = 42;
            intBlock[i] = 42;
            longBlock[i] = 42;
            floatBlock[i] = 42;
            doubleBlock[i] = 42.;
        }
        
        // byte set
        n5.createDataset(byteSetGzip, dimensions, blockSize, DataType.INT8, CompressionType.GZIP);
        final DatasetAttributes attrsByte = n5.getDatasetAttributes(byteSetGzip);
        // short set
        n5.createDataset(shortSetGzip, dimensions, blockSize, DataType.INT16, CompressionType.GZIP);
        final DatasetAttributes attrsShort = n5.getDatasetAttributes(shortSetGzip);
        // int set
        n5.createDataset(intSetGzip, dimensions, blockSize, DataType.INT32, CompressionType.GZIP);
        final DatasetAttributes attrsInt = n5.getDatasetAttributes(intSetGzip);
        // long set
        n5.createDataset(longSetGzip, dimensions, blockSize, DataType.INT64, CompressionType.GZIP);
        final DatasetAttributes attrsLong = n5.getDatasetAttributes(longSetGzip);
        // float set
        n5.createDataset(floatSetGzip, dimensions, blockSize, DataType.FLOAT32, CompressionType.GZIP);
        final DatasetAttributes attrsFloat = n5.getDatasetAttributes(floatSetGzip);
        // double set
        n5.createDataset(doubleSetGzip, dimensions, blockSize, DataType.FLOAT64, CompressionType.GZIP);
        final DatasetAttributes attrsDouble = n5.getDatasetAttributes(doubleSetGzip);

        for(int x = 0; x < blockSize[0]; x++) {
            for(int y = 0; y < blockSize[1]; y++) {
                for(int z = 0; z < blockSize[2]; z++) {
                    
                    // write byte
                    final ByteArrayDataBlock byteDataBlock = new ByteArrayDataBlock(
                        blockSize, new long[]{x, y, z}, byteBlock
                    );
                    n5.writeBlock(byteSetGzip, attrsByte, byteDataBlock);
                    
                    // write short
                    final ShortArrayDataBlock shortDataBlock = new ShortArrayDataBlock(
                        blockSize, new long[]{x, y, z}, shortBlock
                    );
                    n5.writeBlock(shortSetGzip, attrsShort, shortDataBlock);
                    
                    // write int
                    final IntArrayDataBlock intDataBlock = new IntArrayDataBlock(
                        blockSize, new long[]{x, y, z}, intBlock
                    );
                    n5.writeBlock(intSetGzip, attrsInt, intDataBlock);
            
                    // write long
                    final LongArrayDataBlock longDataBlock = new LongArrayDataBlock(
                        blockSize, new long[]{x, y, z}, longBlock
                    );
                    n5.writeBlock(longSetGzip, attrsLong, longDataBlock);
                    
                    // write float
                    final FloatArrayDataBlock floatDataBlock = new FloatArrayDataBlock(
                        blockSize, new long[]{x, y, z}, floatBlock
                    );
                    n5.writeBlock(floatSetGzip, attrsFloat, floatDataBlock);
                    
                    // write double
                    final DoubleArrayDataBlock doubleDataBlock = new DoubleArrayDataBlock(
                        blockSize, new long[]{x, y, z}, doubleBlock
                    );
                    n5.writeBlock(doubleSetGzip, attrsDouble, doubleDataBlock);
                }
            }
        }
    }


    public static void makeEmptyData() throws IOException {
		n5 = N5.openFSWriter(testDir);
        n5.createDataset(emptySet, dimensions, blockSize, DataType.INT32, CompressionType.GZIP);
    }

    
    public static void main(String args[]) throws IOException {
        makeEmptyData();
        makeTestDataRaw();
        makeTestDataGzip();
    }

}
