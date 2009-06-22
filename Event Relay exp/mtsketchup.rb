require "sketchup.rb"
#require "Sketchupmt.dll"


def initialize
        super
        @toolsObserver = MyToolsObserver.new
        Sketchup.active_model.tools.add_observer(@toolsObserver)
        # send Zoom command to open ruby console
       # SendKeyMacro("!WR");