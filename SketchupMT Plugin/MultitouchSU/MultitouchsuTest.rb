#
puts "MultitouchSuTest.rb running from " + Dir.getwd
oldDir = Dir.getwd
Dir.chdir("Debug")
puts "chdir to " + Dir.getwd
require "MultitouchSU.dll"
puts " Invoked require"
Dir.chdir(oldDir)
puts "Dir is now " + Dir.getwd
