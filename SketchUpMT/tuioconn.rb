#$pf = Sketchup.find_support_file("Plugins")


#def start
#  $out = File.open(File.join($pf, "tuio.out"), "w")
 # SKSocket.connect("localhost", 3000)
 # SKSocket.add_socket_listener do |s|
  #  $out.print "#{Time.now.to_i}: "
  #  $out.puts s
  #  $out.flush
 #   if s == "Connection established"
 #   end
 # end
#end

#def stop
 # SKSocket.disconnect
 # $out.flush
 # $out.close
#  $out = nil
# end
   



# Show the Ruby Console at startup so we can see any programming errors we may make.
Sketchup.send_action "showRubyPanel:"





# debug
# puts TUIO_ROOT

UI.menu( "PlugIns" ).add_item( "MultiTouch Single" ) {
  cursors = File.open( File.join( TUIO_ROOT, "tuio_cursor.txt"),"r") do |f|
   f.read  
  end

  # pull out comma separated values
  x_pos, y_pos = cursors.split(", ")

  # sketchup likes actual floats
  x_pos = x_pos.to_f
  y_pos = y_pos.to_f

  # scale these to something larger
  # TODO: not quite sure what ranges these need to be in
  x_pos *= 100
  y_pos *= 100

  # debug
  # UI.messagebox( [x_pos, y_pos].inspect)

  # get the current view
  view = Sketchup.active_model.active_view

  # plug in data
  eye = [x_pos, y_pos, view.camera.eye.z]
  target = [0,0,0]
  up = [0,0,1]

  # create a new camera
  my_camera = Sketchup::Camera.new eye, target, up

  # # Get a handle to the current view and change its camera.
  view.camera = my_camera
}

class MultiTouchAnimation
  def nextFrame(view)
    cursors = File.open( File.join( TUIO_ROOT, "tuio_cursor.txt"),"r") do |f|
      f.read  
    end

    # pull out comma separated values
    x_pos, y_pos = cursors.split(", ")

    # sketchup likes actual floats
    x_pos = x_pos.to_f
    y_pos = y_pos.to_f

    # scale these to something larger
    # TODO: not quite sure what ranges these need to be in
    x_pos *= 100
    y_pos *= 100

    # debug
    # UI.messagebox( [x_pos, y_pos].inspect)
    
    # plug in data
    eye = [x_pos, y_pos, view.camera.eye.z]
    target = [0,0,0]
    up = [0,0,1]

    # create a new camera
    my_camera = Sketchup::Camera.new eye, target, up

    # Get a handle to the current view and change its camera.
    view.camera = my_camera
    
    return true
  end
end
   # This adds an item to the Camera menu to activate our custom animation.
UI.menu( "PlugIns" ).add_item( "MultiTouch Animation" ) {
  Sketchup.active_model.active_view.animation = MultiTouchAnimation.new
}



