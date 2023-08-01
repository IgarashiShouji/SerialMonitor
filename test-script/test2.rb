print "mruby test script 2\n"

th = WorkerThread.new
th.run do
xls = OpenXLSX.new
xls.create('スプレッドシート.xlsx') do
  xls.workbook do
    xls.sheet("Sheet1") do
      xls.setSheetName("test sheet")
      xls.set_value('A3', 'test data')
      xls.set_value('A4', 10)
      xls.set_value('A5', 0.1)
    end
  end
end
xls.open('スプレッドシート.xlsx') do
  xls.workbook do
    xls.sheet("test sheet") do
      printf("A3: %s\n", xls.cell('A3'));
      printf("A4: %d\n", xls.cell('A4'));
      printf("A5: %f\n", xls.cell('A5'));
    end
  end
end
end
th.join

print "mruby test script 2 end\n"
print "\n"
