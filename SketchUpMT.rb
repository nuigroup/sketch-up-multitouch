# ---------------------------------------------
# Test plugin to load MultitouchSu.dll
# ---------------------------------------------
    require "sketchup.rb"
    require "MultitouchSu/MultitouchSU.dll"

    # ----------------------------------------------------------------------------
    class MyTuioClient < TuioClient
    # ----------------------------------------------------------------------------

        # ----------------------------------
        # initialize
        # ----------------------------------
        def initialize
            super();         # <== initialize the Ruby TuioClient extension
            @aSketchUpRect = GetSketchUpRect();
            @aSketchUpViewRect = GetSketchUpViewRect();
            # show SketchUp coordinates in debugging log
            msg = sprintf("SketchUpRect x[%d]y[%d]width[%d]height[%d]",
                @aSketchUpRect[0],@aSketchUpRect[1],@aSketchUpRect[2],@aSketchUpRect[3]);
            puts(msg);
            Logit(msg);
            msg = sprintf("SketchUpViewRect x[%d]y[%d]width[%d]height[%d]",
                @aSketchUpViewRect[0],@aSketchUpViewRect[1],@aSketchUpViewRect[2],@aSketchUpViewRect[3]);
            puts(msg);
            Logit(msg);
            @SuViewX = @aSketchUpViewRect[0];
            @SuViewY = @aSketchUpViewRect[1];
            @SuViewWidth = @aSketchUpViewRect[2];
            @SuViewHeight = @aSketchUpViewRect[3];

            @aperture = nil;
            @model = Sketchup.active_model
            @view = @model.active_view
            @ph = @view.pick_helper
            @ip = Sketchup::InputPoint.new
            @ipXY = [0,0];
       end
        # ----------------------------------
        #       PickHelper
        # ----------------------------------
        # load the EventRelay Ruby extension

        def phCursorDown(flags, x, y, view)
            entity = entity_under_mouse(view, x, y)
            @xdown = x
            @ydown = y
            if entity && selection_include?(entity)
                @mode_erase = true
            else
                @mode_erase = false
            end
        end

        #Identify the entity under the mouse
        def entity_under_mouse(view, x, y)
            (@aperture) ? @ph.do_pick(x, y, @aperture) : @ph.do_pick(x, y)
            entity = @ph.best_picked
            @entity = nil
            if entity
                @ip.pick view, x, y
                @ipXY = [x,y];
                @entity = entity
                puts "entity: #{@entity}"
            end
        end

        #Move halfway to point
        def move_halfway_toward(entity, target_pt)
            begin
                
                # Undo our current transform with inverse,
                # then move to the halfway point.
                entity.transform! entity.transformation.inverse;
                entity.transform! Geom::Transformation.new halfway;
                # Invalidate our view to force SU to refresh the screen.
                Sketchup.active_model.active_view.invalidate;
            rescue Exception => e
                puts e.message
                e.backtrace.each{|line| puts "\t" + line }
            end;
        end

        #Move entity to point
        def move_toward(entity, target_pt)
            begin
                current_pt = entity.transformation.origin;
                halfway = Geom::Point3d.linear_combination(1.0, current_pt, 1.0,
                    target_pt)
                # Undo our current transform with inverse,
                # then move to the halfway point.
                entity.transform! entity.transformation.inverse
                entity.transform! Geom::Transformation.new halfway
                # Invalidate our view to force SU to refresh the screen.
                Sketchup.active_model.active_view.invalidate
            rescue Exception => e
                puts e.message
                e.backtrace.each{|line| puts "\t" + line }
            end;
        end

        # ----------------------------------
        #       OnTuioData (called by TuioClient when TUIO data is available)
        # ----------------------------------
        #def OnTuioData( stringOfTuioData )
            ##Logit "FROM RUBY OnTuioData: #{stringOfTuioData}"
            #puts stringOfTuioData; #write to SketchUp console
            #return true
        #end

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

        def OnTuioAddObject( symbolID, sessionID, positionX, positionY, angle)
            puts "AddObject: #{symbolID} #{sessionID} #{positionX} #{positionY} #{angle}"
            return true
        end;

        def OnTuioSetObject( symbolID, sessionID, positionX, positionY, angle,
                motionSpeed, rotationSpeed, motionAccel, rotationAccel)
            puts "SetObject: #{symbolID} #{sessionID} #{positionX} #{positionY} #{angle} #{motionSpeed} #{rotationSpeed} #{motionAccel} #{rotationAccel}"
            return true
        end;

        def OnTuioDelObject( symbolID, sessionID)
            puts "DelObject: #{symbolID} #{sessionID}";
            return true
        end;

        def OnTuioAddCursor( symbolID, sessionID, positionX, positionY )
            puts "AddCursor: #{symbolID} #{sessionID} #{positionX} #{positionY}"
            x,y = GetSuViewXY(positionX,positionY);
            msg = sprintf("ViewX[%d] ViewY[%d]", x, y);
            puts msg;
            flags = 0;
            phCursorDown(flags, x, y, @view);
            return true
        end

        def OnTuioSetCursor( symbolID, sessionID, positionX, positionY, motionSpeed, motionAccel )
            #puts "SetCursor: #{symbolID} #{sessionID} #{positionX} #{positionY} #{motionSpeed} #{motionAccel}"
            x,y = GetSuViewXY(positionX,positionY);
            msg = sprintf("ViewX[%d] ViewY[%d]", x, y);
            puts msg;
            # move the chosen entity to the new coordinates
            if (@entity)
                xDelta = x-@ipXY[0];
                yDelta = y-@ipXY[1];
                #moveto_pt = Geom::Point3d.new( xDelta, yDelta, 0);
                #move_halfway_toward(@entity, moveto_pt)
                msg = sprintf("Deltas[%d,%d] ipXY[%d,%d]", xDelta, yDelta, @ipXY[0],@ipXY[1]);
                puts(msg);
                #if (xDelta): xDelta = (xDelta<0)? -1:1; end;
                #if (yDelta): yDelta = (yDelta<0)? -1:1; end;
                moveto_pt = Geom::Point3d.new( xDelta, yDelta, 0);
                move_toward(@entity, moveto_pt)
                @ipXY[0] = x;
                @ipXY[1] = y;
            end;
            return true
        end

        def OnTuioDelCursor( symbolID, sessionID)
            puts "DelCursor: #{symbolID} #{sessionID}";
            return true
        end

    end # class MyTuioClient

    #end of script
 unless file_loaded?("MultitouchSuPlgn.rb")
    puts " Invoked require MultitouchSu.dll"
    # ------------------------------------------------------------------------
    # The MultitouchSu.dll Ruby extension will see the next statement and make
    # a global reference to "myTuioClient" object class so it lives past
    # the termination of this script. Anything outside MyTuioClient class will
    # be eaten by the Ruby garbage collector.
    # ------------------------------------------------------------------------
    myTuioClient = MyTuioClient.new();
    ## UI.menu("Plugins").add_item("KB CmdConfig") { myERUserCommands.KbCmdConfig() }
    file_loaded "MultitouchSuPlgn.rb"
 end
