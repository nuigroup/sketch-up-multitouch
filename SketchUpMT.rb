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
        end
        # ----------------------------------
        #	OnTuioData (called by TuioClient when TUIO data is available)
        # ----------------------------------
        #def OnTuioData( stringOfTuioData )
            ##Logit "FROM RUBY OnTuioData: #{stringOfTuioData}"
            #puts stringOfTuioData; #write to SketchUp console
            #return true
        #end

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
            return true
        end

        def OnTuioSetCursor( symbolID, sessionID, positionX, positionY, motionSpeed, motionAccel )
            puts "SetCursor: #{symbolID} #{sessionID} #{positionX} #{positionY} #{motionSpeed} #{motionAccel}"
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
