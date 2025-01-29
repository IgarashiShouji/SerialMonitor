def testCreateSpradSheet(fname)
  xls = OpenXLSX.new
  if !Core.exists(fname) then
      xls.create(fname) do
        xls.sheet("Sheet1") do
          xls.setSheetName("test sheet 1")
          xls.set_value('A3', 'test data 1')
          xls.set_value('A4', 10)
          xls.set_value('A5', 0.1)
        end
        xls.sheet("test sheet 2") do
          xls.set_value('A3', 'test data 2')
          xls.set_value('A4', 109)
          xls.set_value('A5', 10.1)
        end
      end
  end
end
def testReadSpradSheet(fname)
  xls = OpenXLSX.new
  xls.open(fname) do
    (xls.sheet_names).each do |sheet_name|
      print "sheet name: ", sheet_name, "\n"
      xls.sheet(sheet_name) do
        value = xls.cell('A3'); if nil != value then; printf("A3: %s\n", value); end
        value = xls.cell('A4'); if nil != value then; printf("A4: %d\n", value); end
        value = xls.cell('A5'); if nil != value then; printf("A5: %f\n", value); end
      end
      true
    end
  end
end

print "mruby test script 2\n"
testCreateSpradSheet( 'スプレッドシート.xlsx' )
testReadSpradSheet(   'スプレッドシート.xlsx' )
print "mruby test script 2 end\n"
print "\n"
