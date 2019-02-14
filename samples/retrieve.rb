require 'open-uri'
samples_dir = File.dirname(__FILE__)
base_url = "https://www.cs.princeton.edu/~appel/modern/testcases/"
files = (1..49).to_a.map { |n| "test#{n}.tig" }
other_files = ['queens.tig', 'merge.tig']
files.concat other_files
threads = []
files.each do |f|
  threads << Thread.new do
    open(File.join(base_url, f)) do |h|
      File.write(File.join(samples_dir, f), h.read)
    end
  end
end

threads.each(&:join)
