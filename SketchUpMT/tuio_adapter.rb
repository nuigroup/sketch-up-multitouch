require 'rubygems'
require 'tuio_client'


@tc = TuioClient.new

@tc.on_cursor_update do | cursor |
  puts "writing #{cursor.x_pos}, #{cursor.y_pos} to file"
  
  File.open("tuio_cursor.txt", "w") do | f |
    f.write("#{cursor.x_pos}, #{cursor.y_pos}")
  end
end

@tc.start

sleep