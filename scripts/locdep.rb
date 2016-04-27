#!/usr/bin/env ruby
# Based on https://code.google.com/p/stacked-crooked/
# Usage
# localize_dependencies.rb input_file output_directory

# Algorithm
# ---------
# Get list of input file dependencies
# Copy each dependency to output directory
# Update input_file paths to local dir (relative to @executable_path)
# Repeat above procedure for input file dependenc


require 'ftools'
require 'fileutils'
require 'open3'

# Runs a subprocess.
# +command+:: command line string to be executed by the system
# +outpipe+:: +Proc+ object that takes the stdout pipe (of type +IO+) as first and only parameter.
# +errpipe+:: +Proc+ object that takes the stderr pipe (of type +IO+) as first and only parameter.
def execute_and_handle(command, outpipe, errpipe)
  Open3.popen3(command) do |_, stdout, stderr|
    if (outpipe)
      outpipe.call(stdout)
    end
    if (errpipe)
      errpipe.call(stderr)
    end
  end
end


def needs_localization(file_path)
  return file_path.index("@executable_path") == nil && file_path.index("/usr/lib/") == nil
end


def get_input_file_dependencies(input_file)
  result = []
  execute_and_handle("otool -L #{input_file}",
                     Proc.new { |outPipe|
                       outPipe.each_line do |line|
                         if line =~ /\s+(.+dylib)\s.+/
                           filename = $1
                           if (needs_localization(filename))
                             result.push($1)
                           end
                         end
                       end
                     },
                     nil)

  if result.length > 0
    puts("Dependencies for #{input_file}:")
    result.each { |dep| puts "  #{dep}" }
    puts
  end
  return result
end


def copy_dependencies(input_file, output_directory)
    puts "copy_dependencies input=#{input_file} output=#{output_directory}"
  FileUtils.mkdir_p output_directory
  get_input_file_dependencies(input_file).each do |dependency|
    target = File.join(output_directory, File.basename(dependency))
    if not File.exists?(target)
      if File.exists?(dependency)
        if dependency.index("#{File.basename(ARGV[0])}") == nil
          FileUtils.cp(dependency, target)
        else
          puts("Skipping #{dependency}.")
        end
      else
        puts("Warning: #{dependency} was not found and is not copied to the output directory.")
      end
      copy_dependencies(target, output_directory)
    end
    update_path(input_file, dependency, output_directory)
    #update_id(target, output_directory)
    puts
    puts
  end
end


def update_path(input_file, old_install_name, output_directory)
  command = "install_name_tool -change #{old_install_name} @executable_path/#{File.join(output_directory, File.basename(old_install_name))} #{input_file}"
  puts command
  execute_and_handle(command, nil, nil)
end


def update_id(input_file, output_directory)
  command = "install_name_tool -id @executable_path/#{File.join(output_directory, File.basename(input_file))} #{input_file}"
  puts command
  execute_and_handle(command, nil, nil)
end

if ARGV.length == 2
    copy_dependencies(ARGV[0], ARGV[1])
else
    $stderr.puts "Usage: ruby localize_dependencies.rb input_file output_directory\nlocalize_dependencies.rb Contents/MacOS/Mesmerize Contents"
end
