import java.nio.file.Path;
import java.nio.charset.StandardCharsets;
import java.util.Scanner;
import java.io.IOException;

public class exe{
    public static void main(String[] args)throws IOException{
        Scanner in = new Scanner(
            Path.of("20260902.md"),
            StandardCharsets.UTF_8
        );
        while(in.hasNextLine()){
            String line=in.nextLine();
            System.out.println(line);
        }
        in.close();
    } 
}