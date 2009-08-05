# ---------------------------------------------
# Test plugin to run MultitouchSu.dll
# ---------------------------------------------
    require "sketchup.rb"
    require "MultitouchSu/MultitouchSU.dll"

class MtSuPanOrbitZoom < TuioClient

        # set this to true to invert the Zoom direction
        ZOOM_INVERT = false

        # Orbit: camera's ray maximum distance to entity
        # to use that entity as the rotation center
        ORBIT_TARGET_MAX_DISTANCE = 1000

        # resolution divider to determine how far to
        # rotate in X and Y relative to tuio cursor movement
        # assume 800dpi; 0.25 rotation per inch
        XY_ROTATION_RESOLUTION = 3200

        # resolution divider to determine how far to
        # rotate in Z relative to tuio cursor rotation
        # assume 128 CPR; 1:2 rotation ratio
        Z_ROTATION_RESOLUTION = 256

        # Distance along the camera direction to use as a
        # rotation center for X/Y pan (horizon point)
        PAN_CENTER_DISTANCE = 1000000

        # zoom resolution
        # assume 2% zoom per X,Y unit
        ZOOM_RESOLUTION = 50

        def initialize
        super();         # <== initialize the Ruby TuioClient extension
        @aSketchUpRect = GetSketchUpRect();
        @aSketchUpViewRect = GetSketchUpViewRect();
        # show SketchUp coordinates in debugging log
        msg = sprintf("SketchUpRect x[%d]y[%d]width[%d]height[%d]",
            @aSketchUpRect[0],@aSketchUpRect[1],@aSketchUpRect[2],@aSketchUpRect[3]);
        Logit(msg);
        msg = sprintf("SketchUpViewRect x[%d]y[%d]width[%d]height[%d]",
            @aSketchUpViewRect[0],@aSketchUpViewRect[1],@aSketchUpViewRect[2],@aSketchUpViewRect[3]);
        Logit(msg);
        @SuViewX = @aSketchUpViewRect[0];
        @SuViewY = @aSketchUpViewRect[1];
        @SuViewWidth = @aSketchUpViewRect[2];
        @SuViewHeight = @aSketchUpViewRect[3];
        @numTuioCursors = 0;
                # mode: orbit
                @pan_zoom_mode = false
        activate();
        end

    def OnTuioAddCursor( symbolID, sessionID, positionX, positionY )
        puts "AddCursor: #{symbolID} #{sessionID} #{positionX} #{positionY}"
        x,y = GetSuViewXY(positionX,positionY);
        msg = sprintf("ViewX[%d] ViewY[%d]", x, y);
        puts msg;
        flags = 0;
        OnTuioCursorDown( x, y);
        return true
    end

    def OnTuioSetCursor( symbolID, sessionID, positionX, positionY, motionSpeed, motionAccel )
        #puts "SetCursor: #{symbolID} #{sessionID} #{positionX} #{positionY} #{motionSpeed} #{motionAccel}"
        x,y = GetSuViewXY(positionX,positionY);
        msg = sprintf("ViewX[%d] ViewY[%d]", x, y);
        puts msg;
        OnMtMove(@numTuioCursors, x, y, @view);
        return true
    end

    def OnTuioDelCursor( symbolID, sessionID)
        puts "DelCursor: #{symbolID} #{sessionID}";
        OnTuioCursorUp();
        return true
    end

        def activate
        # Called from AppObserver
                # store the view & model for manipulation during rotation
                @model = Sketchup.active_model
                @view = @model.active_view
        end

        def OnTuioCursorDown( x, y)

        @numTuioCursors += 1;
        if (@numTuioCursors > 3):
            @numTuioCursors = 3; end;
        if (@numTuioCursors == 1):
            OnMtPan_or_Orbit(@numTuioCursors, x, y, @view);end;
        if (@numTuioCursors == 3):
            OnMtPan_or_Orbit(@numTuioCursors, x, y, @view);end;
        Logit("Fingers Down:#{@numTuioCursors}");
        if ( 1 == @numTuioCursors)
            @curs1_x = x;
            @curs1_y = y;
        end
        end

        def OnTuioCursorUp()

        @numTuioCursors -= 1;
        if (@numTuioCursors < 0):
            @numTuioCursors = 0; end;
        Logit("Fingers Up:#{@numTuioCursors}");
        end

    # Convert TUIO to SketchUp view coordinates
    def GetSuViewXY(tuiox, tuioy)
        #Updatae SketchUp View rect in case user resized it.
        @aSketchUpViewRect = GetSketchUpViewRect();
        @SuViewX = @aSketchUpViewRect[0];
        @SuViewY = @aSketchUpViewRect[1];
        @SuViewWidth = @aSketchUpViewRect[2];
        @SuViewHeight = @aSketchUpViewRect[3];
        x = Integer(@SuViewWidth * tuiox);
        y = Integer(@SuViewHeight * tuioy);
        return [x,y];
    end

    # ---------------------------------------
    # Pan or Orbit
    # ---------------------------------------
        def OnMtPan_or_Orbit(fingers, x, y, view)

                # initialise last x/y values
                @last_x = x
                @last_y = y

                # find the rotation center;
                # either the first connected object in the camera's ray within a given distance
                # else use the center of the whole model

                camera = @view.camera
                camray = [camera.eye, camera.direction]

                # test if there are items in the camera's ray
                if(item = @model.raytest camray)

                        # split item into location point & entities chain
                        location = item[0]
                        entities = item[1]

                        # find the distance to the item
                        distance = camera.eye.distance(location)

                        # check if the entity is an edge or face within the maximum distance
                        if ((entities[0].typename == "Face") || (entities[0].typename == "Edge") && (distance < ORBIT_TARGET_MAX_DISTANCE))

                                # find all connected entities and add their points to a bounding box
                                connected = entities[0].all_connected
                                bb = bb =  Geom::BoundingBox.new

                                # loop through all vertices of all entites and add to the bounding box
                                connected.each do |entity|
                                        entity.vertices.each do |vertex|
                                                bb.add vertex.position  # position is the point3d of the vertex
                                        end
                                end

                                # rotate around the center of the bounding box
                                @rot_center = bb.center

                        else

                                # item is too far away or wrong type
                                # rotate around the center of the model instead
                                @rot_center = @model.bounds.center

                        end

                else

                        # nothing in camera ray;
                        # rotate around the center of the model
                        @rot_center = @model.bounds.center

                end

        end

    # ---------------------------------------
    # Pan or Orbit
    # ---------------------------------------
        def OnMtMove(fingers, x, y, view)

        # Zooming
        if ( fingers == 2)
            OnMtZoom(fingers, x, y, view);

        else
            # set the mode pan == 1 finger; zoom == 2 fingers; orbit = 3 fingers
            @pan_zoom_mode = (fingers < 3 );

            # only do the pan or x/y rotate when fingers (tuio cursors) are down
            if( fingers > 0)

                # get the cursor movement deltas
                dx = x - @last_x
                dy = y - @last_y


                # get the current camera
                camera = @view.camera

                # pan if one finger
                if @pan_zoom_mode

                    # X/Y pan

                    # rotate around a distant point in the camera's direction
                    rot_center = camera.eye.offset(camera.direction, PAN_CENTER_DISTANCE)

                    # ideally pan 1:1 with tuio cursor movement
                    scale = view.pixels_to_model(1, @model.bounds.center)
                    atan = Math::atan(scale / PAN_CENTER_DISTANCE)

                    angle_x = dx * atan
                    angle_y = dy * atan

                else
                    # X/Y rotate

                    # rotate around the center calculated on first tuio cursor down
                    rot_center = @rot_center

                    # rotation angles depend on tuio cursor movement deltas
                    angle_x = dx * 2 * Math::PI / XY_ROTATION_RESOLUTION
                    angle_y = dy * 2 * Math::PI / XY_ROTATION_RESOLUTION

                end

                # get rotation vectors
                @vector_x = @view.camera.yaxis.reverse!
                @vector_y = @view.camera.xaxis.reverse!

                # create the rotation transformations for the camera 'eye' vector
                t_x = Geom::Transformation.rotation(rot_center, @vector_x, angle_x)
                t_y = Geom::Transformation.rotation(rot_center, @vector_y, angle_y)

                # apply the transformations
                t_eye = t_x * t_y * camera.eye
                t_target = t_x * t_y * camera.target
                t_up = t_y * camera.up

                # set the camera
                camera.set(t_eye, t_target, t_up)
            end
        end

                # save base values for next move
                # regardless of mode
                @last_x = x
                @last_y = y

        end

    # ---------------------------------------
        # Zoom
    # ---------------------------------------
        def OnMtZoom(fingers, x, y, view)

                # get the current camera
                camera = @view.camera

        xDelta = (@last_x - @curs1_x).abs-(x - @curs1_x).abs;
        yDelta = (@last_y - @curs1_y).abs-(y - @curs1_y).abs;
        zoom_direction = ((xDelta + yDelta)>0)? -1 : 1;

                # zoom if two fingers
                if (@pan_zoom_mode = (fingers == 2) )

                        # zoom vector is relative to the current cursor position
                        zoom_vector = @view.pickray(@last_x, @last_y)[1]

                        # reverse the direction if ZOOM_INVERT is true
                        if ZOOM_INVERT
                                zoom_vector.reverse!
                        end

                        # zoom distance is proportional to the distance between the camera eye and target
                        zoom_distance = camera.eye.distance(camera.target) * zoom_direction / ZOOM_RESOLUTION

                        # translate both the camera eye and target along the zoom vector
                        t_eye = camera.eye.offset(zoom_vector, zoom_distance)
                        t_target = camera.target.offset(zoom_vector, zoom_distance)
                        t_up = camera.up                # no transform

                else

                        # create the rotation transformation for the camera 'up' vector
                        # rotation vector is perpendicular to the screen / viewport
                        vector_z = camera.zaxis

                        # reverse the direction unless ZOOM_INVERT is true
                        if !ZOOM_INVERT
                                vector_z.reverse!
                        end

                        # rotation angles depend on tuio cursor delta
                        angle_z = zoom_direction * 2 * Math::PI / Z_ROTATION_RESOLUTION

                        # create the rotation transformations for the camera 'up' vector
                        t_z = Geom::Transformation.rotation(camera.eye, vector_z, angle_z)

                        # apply the transformation
                        t_eye = camera.eye              # no transform
                        t_target = camera.target        # no transform
                        t_up = t_z * camera.up

                end

                # set the camera
                camera.set(t_eye, t_target, t_up)

        end
end; #class MtSuPanOrbitZoom

# ----------------------------------------------
class MtSuAppObserver < Sketchup::AppObserver
# ----------------------------------------------

    def initialize(instOfMtPanOrbitZoom)
    @myMtPanOrbitZoom = instOfMtPanOrbitZoom;
    end;

    def onNewModel(a)
        @myMtPanOrbitZoom.activate();
    end


    def onOpenModel(a)
        @myMtPanOrbitZoom.activate();
    end

end; # class MtSuAppObserver

##debugger();
   unless file_loaded?(__FILE__)
    # ------------------------------------------------------------------------
    # The MultitouchSu.dll Ruby extension will see the next statement and make
    # a global reference to "MtSuPanOrbitZoom" object class so it lives past
    # the termination of this script. Anything outside MtSuPanOrbitZoom class will
    # be eaten by the Ruby garbage collector.
    # ------------------------------------------------------------------------
    myMtSuPanOrbitZoom = MtSuPanOrbitZoom.new();
    myMtSuAppObserver = MtSuAppObserver.new(myMtSuPanOrbitZoom);

      file_loaded(__FILE__);
   end
