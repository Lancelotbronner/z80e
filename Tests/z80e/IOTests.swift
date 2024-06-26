import XCTest
import XCTz80e
import z80e

final class IOTests: XCTestCaseTI83p {

	//MARK: - OUT

	func test_OUT_n_A() {
		let test: [UInt8] = [0xD3, 0x12] // OUT (0x12), A
		asic.cpu.registers.A = 0x3C;
		flash(test)
		_value = 0
		let cycles = cpu_execute(&asic.cpu, 11)
		XCTAssertEqual(_value, 0x3C)
		XCTAssertEqual(cycles, 0)
	}

	func test_OUT_C_0() {
		// Note: Despite the mnomic of this instruction, the
		// CMOS z80 variant (which TI calculators use) output
		// 0xFF instead.
		let test: [UInt8] = [0xED, 0x71] // OUT (C), 0
		asic.cpu.registers.C = 0x12;
		flash(test)
		cpu_execute(&asic.cpu, 12)
		XCTAssertEqual(_value, 0xFF)
	}

	func test_OUT_C_r() {
		let test: [UInt8] = [0xED, 0x41] // OUT (C), B
		asic.cpu.registers.B = 0xB2;
		asic.cpu.registers.C = 0x12;
		flash(test)
		cpu_execute(&asic.cpu, 12)
		XCTAssertEqual(_value, 0xB2)
	}

	//MARK: - IN

	func test_IN_A_n() {
		let test: [UInt8] = [0xDB, 0x12] // IN A, (0x12)
		flash(test)
		_value = 0x3C
		let cycles = cpu_execute(&asic.cpu, 11)
		XCTAssertEqual(asic.cpu.registers.A, 0x3C)
		XCTAssertEqual(cycles, 0)
	}

	func test_IN_C() {
		let test: [UInt8] = [0xED, 0x70] // IN (C)
		flash(test)
		_value = 0
		asic.cpu.registers.C = 0x12;
		let cycles = cpu_execute(&asic.cpu, 12)
		XCTAssertEqual(asic.cpu.registers.flags.Z, 1)
		XCTAssertEqual(cycles, 0)
	}

	func test_IN_r_C() {
		let test: [UInt8] = [0xED, 0x40] // IN B, (C)
		flash(test)
		_value = 0x5B
		asic.cpu.registers.C = 0x12;
		let cycles = cpu_execute(&asic.cpu, 12)
		XCTAssertEqual(asic.cpu.registers.B, 0x5B)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - INI

	func test_INI() {
		let test: [UInt8] = [0xED, 0xA2] // INI
		_value = 0x3E
		asic.cpu.registers.B = 1;
		asic.cpu.registers.HL = 0xC000;
		asic.cpu.registers.C = 0x12;
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 16)
		XCTAssertEqual(read_byte(0xC000), 0x3E)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xC001)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - IND

	func test_IND() {
		let test: [UInt8] = [0xED, 0xAA] // IND
		_value = 0x3E
		asic.cpu.registers.B = 1;
		asic.cpu.registers.HL = 0xC000;
		asic.cpu.registers.C = 0x12;
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 16)
		XCTAssertEqual(read_byte(0xC000), 0x3E)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xBFFF)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - INIR

	func test_INIR() {
		let test: [UInt8] = [0xED, 0xB2] // INIR
		_value = 0x3E
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC000;
		asic.cpu.registers.C = 0x12;
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 100)
		XCTAssertEqual(read_byte(0xC000), 0x3E)
		XCTAssertEqual(read_byte(0xC001), 0x3E)
		XCTAssertEqual(read_byte(0xC002), 0x3E)
		XCTAssertEqual(read_byte(0xC003), 0x3E)
		XCTAssertEqual(read_byte(0xC004), 0x3E)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xC005)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - INDR

	func test_INDR() {
		let test: [UInt8] = [0xED, 0xBA] // INDR
		_value = 0x3E
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC004;
		asic.cpu.registers.C = 0x12;
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 100)
		XCTAssertEqual(read_byte(0xC000), 0x3E)
		XCTAssertEqual(read_byte(0xC001), 0x3E)
		XCTAssertEqual(read_byte(0xC002), 0x3E)
		XCTAssertEqual(read_byte(0xC003), 0x3E)
		XCTAssertEqual(read_byte(0xC004), 0x3E)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xBFFF)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - OUTI

	func test_OUTI() {
		let test: [UInt8] = [0xED, 0xA3]
		_value = 0x00
		asic.cpu.registers.C = 0x12;
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC000;
		write_byte(0xC000, 0x11)
		write_byte(0xC001, 0x22)
		write_byte(0xC002, 0x33)
		write_byte(0xC003, 0x44)
		write_byte(0xC004, 0x55)
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 16)
		XCTAssertEqual(_value, 0x11)
		XCTAssertEqual(asic.cpu.registers.B, 4)
		XCTAssertEqual(asic.cpu.registers.HL, 0xC001)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - OUTD

	func test_OUTD() {
		let test: [UInt8] = [0xED, 0xAB]
		_value = 0x00
		asic.cpu.registers.C = 0x12;
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC004;
		write_byte(0xC000, 0x11)
		write_byte(0xC001, 0x22)
		write_byte(0xC002, 0x33)
		write_byte(0xC003, 0x44)
		write_byte(0xC004, 0x55)
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 16)
		XCTAssertEqual(_value, 0x55)
		XCTAssertEqual(asic.cpu.registers.B, 4)
		XCTAssertEqual(asic.cpu.registers.HL, 0xC003)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - OTIR

	func test_OTIR() {
		let test: [UInt8] = [0xED, 0xB3]
		_value = 0x00
		asic.cpu.registers.C = 0x12;
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC000;
		write_byte(0xC000, 0x11)
		write_byte(0xC001, 0x22)
		write_byte(0xC002, 0x33)
		write_byte(0xC003, 0x44)
		write_byte(0xC004, 0x55)
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 100)
		XCTAssertEqual(_value, 0x55)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xC005)
		XCTAssertEqual(cycles, 0)
	}

	//MARK: - OTDR

	func test_OTDR() {
		let test: [UInt8] = [0xED, 0xBB]
		_value = 0x00
		asic.cpu.registers.C = 0x12;
		asic.cpu.registers.B = 5;
		asic.cpu.registers.HL = 0xC004;
		write_byte(0xC000, 0x11)
		write_byte(0xC001, 0x22)
		write_byte(0xC002, 0x33)
		write_byte(0xC003, 0x44)
		write_byte(0xC004, 0x55)
		flash(test)
		let cycles = cpu_execute(&asic.cpu, 100)
		XCTAssertEqual(_value, 0x11)
		XCTAssertEqual(asic.cpu.registers.B, 0)
		XCTAssertEqual(asic.cpu.registers.HL, 0xBFFF)
		XCTAssertEqual(cycles, 0)
	}

}
