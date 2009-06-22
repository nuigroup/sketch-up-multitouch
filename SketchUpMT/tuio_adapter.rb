# do not resort to this soln until event relay or DRb solution works
CLIENT_ROOT = File.join( File.dirname( __FILE__ ), "client" )

# adding lib folder to ruby path
$: << File.join( CLIENT_ROOT, "lib" )

require 'tuio_client'


@tc = TuioClient.new

@tc.on_cursor_update do | cursor |
  puts "writing TUIO cur. coordinates #{cursor.x_pos}, #{cursor.y_pos} to txt"
  #Appending TUIO data to file 
  File.open("tuio_cursor.txt", "w") do | f |
    f.write("#{cursor.x_pos}, #{cursor.y_pos}")
  end
end

@tc.start

sleep


