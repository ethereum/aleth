#include "Arith256.h"

#include <iostream>
#include <iomanip>

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/IntrinsicInst.h>
#include "preprocessor/llvm_includes_end.h"

#include "Type.h"
#include "Endianness.h"
#include "Utils.h"

namespace dev
{
namespace eth
{
namespace jit
{

Arith256::Arith256(IRBuilder& _builder) :
	CompilerHelper(_builder)
{}

void Arith256::debug(llvm::Value* _value, char _c, llvm::Module& _module, IRBuilder& _builder)
{
	static const auto funcName = "debug";
	auto func = _module.getFunction(funcName);
	if (!func)
		func = llvm::Function::Create(llvm::FunctionType::get(Type::Void, {Type::Word, _builder.getInt8Ty()}, false), llvm::Function::ExternalLinkage, funcName, &_module);

	_builder.CreateCall(func, {_builder.CreateZExtOrTrunc(_value, Type::Word), _builder.getInt8(_c)});
}

namespace
{
llvm::Function* generateLongMulFunc(char const* _funcName, llvm::IntegerType* _ty, llvm::IntegerType* _wordTy, llvm::Module& _module)
{
	auto func = llvm::Function::Create(llvm::FunctionType::get(_ty, {_ty, _ty}, false), llvm::Function::PrivateLinkage, _funcName, &_module);
	func->setDoesNotAccessMemory();
	func->setDoesNotThrow();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto entryBB = llvm::BasicBlock::Create(func->getContext(), "Entry", func);
	auto outerLoopHeaderBB = llvm::BasicBlock::Create(func->getContext(), "OuterLoop.Header", func);
	auto innerLoopBB = llvm::BasicBlock::Create(func->getContext(), "InnerLoop", func);
	auto outerLoopFooterBB = llvm::BasicBlock::Create(func->getContext(), "OuterLoop.Footer", func);
	auto exitBB = llvm::BasicBlock::Create(func->getContext(), "Exit", func);

	auto builder = IRBuilder{entryBB};
	auto dwordTy = builder.getIntNTy(_wordTy->getBitWidth() * 2);
	auto indexTy = builder.getInt32Ty();
	auto _0 = builder.getInt32(0);
	auto _1 = builder.getInt32(1);
	auto wordMask = builder.CreateZExt(llvm::ConstantInt::get(_wordTy, -1, true), dwordTy);
	auto wordBitWidth = builder.getInt32(_wordTy->getBitWidth());
	auto dim = builder.getInt32(_ty->getBitWidth() / _wordTy->getBitWidth()); // FIXME: assert about word type
	builder.CreateBr(outerLoopHeaderBB);

	auto extractWordAsDword = [&](llvm::Value* _a, llvm::Value* _idx, llvm::Twine const& _name)
	{
		auto word = builder.CreateLShr(_a, builder.CreateZExt(builder.CreateNUWMul(_idx, wordBitWidth), _ty));
		word = builder.CreateAnd(builder.CreateTrunc(word, dwordTy), wordMask, _name);
		return word;
	};

	builder.SetInsertPoint(outerLoopHeaderBB);
	auto j = builder.CreatePHI(indexTy, 2, "j");
	auto p = builder.CreatePHI(_ty, 2, "p");
	auto yj = extractWordAsDword(y, j, "y.j");
	auto iEnd = builder.CreateSub(dim, j, "i.end", true, true);
	builder.CreateBr(innerLoopBB);

	builder.SetInsertPoint(innerLoopBB);
	auto i = builder.CreatePHI(indexTy, 2, "i");
	auto pInner = builder.CreatePHI(_ty, 2, "p.inner");
	auto carry = builder.CreatePHI(_wordTy, 2, "carry");

	auto k = builder.CreateNUWAdd(i, j, "k");
	auto offset = builder.CreateZExt(builder.CreateNUWMul(k, wordBitWidth), _ty, "offset");
	auto xi = extractWordAsDword(x, i, "x.i");
	auto m = builder.CreateNUWMul(xi, yj, "m");
	auto mask = builder.CreateShl(builder.CreateZExt(wordMask, _ty), offset, "mask");
	auto nmask = builder.CreateXor(mask, llvm::ConstantInt::get(_ty, -1, true), "nmask");
	auto w = builder.CreateTrunc(builder.CreateLShr(pInner, offset), dwordTy);
	w = builder.CreateAnd(w, wordMask, "w");
	auto s = builder.CreateAdd(w, builder.CreateZExt(carry, dwordTy), "s.wc", true, true);
	s = builder.CreateNUWAdd(s, m, "s");
	auto carryNext = builder.CreateTrunc(builder.CreateLShr(s, llvm::ConstantInt::get(dwordTy, _wordTy->getBitWidth())), _wordTy, "carry.next");
	auto wNext = builder.CreateAnd(s, wordMask);
	auto pMasked = builder.CreateAnd(pInner, nmask, "p.masked");
	auto pNext = builder.CreateOr(builder.CreateShl(builder.CreateZExt(wNext, _ty), offset), pMasked, "p.next");

	auto iNext = builder.CreateNUWAdd(i, _1, "i.next");
	auto innerLoopCond = builder.CreateICmpEQ(iNext, iEnd, "i.cond");
	builder.CreateCondBr(innerLoopCond, outerLoopFooterBB, innerLoopBB);
	i->addIncoming(_0, outerLoopHeaderBB);
	i->addIncoming(iNext, innerLoopBB);
	pInner->addIncoming(p, outerLoopHeaderBB);
	pInner->addIncoming(pNext, innerLoopBB);
	carry->addIncoming(llvm::ConstantInt::get(_wordTy, 0), outerLoopHeaderBB);
	carry->addIncoming(carryNext, innerLoopBB);

	builder.SetInsertPoint(outerLoopFooterBB);
	auto jNext = builder.CreateNUWAdd(j, _1, "j.next");
	auto outerLoopCond = builder.CreateICmpEQ(jNext, dim, "j.cond");
	builder.CreateCondBr(outerLoopCond, exitBB, outerLoopHeaderBB);
	j->addIncoming(_0, entryBB);
	j->addIncoming(jNext, outerLoopFooterBB);
	p->addIncoming(llvm::ConstantInt::get(_ty, 0), entryBB);
	p->addIncoming(pNext, outerLoopFooterBB);

	builder.SetInsertPoint(exitBB);
	builder.CreateRet(pNext);

	return func;
}
}


llvm::Function* Arith256::getMulFunc(llvm::Module& _module)
{
	static const auto funcName = "evm.mul.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	return generateLongMulFunc(funcName, Type::Word, Type::Size, _module);
}

llvm::Function* Arith256::getMul512Func(llvm::Module& _module)
{
	static const auto funcName = "evm.mul.i512";
	if (auto func = _module.getFunction(funcName))
		return func;

	auto i512Ty = llvm::IntegerType::get(_module.getContext(), 512);
	return generateLongMulFunc(funcName, i512Ty, Type::Size, _module);
}

namespace
{
llvm::Function* createUDivRemFunc(llvm::Type* _type, llvm::Module& _module, char const* _funcName)
{
	// Based of "Improved shift divisor algorithm" from "Software Integer Division" by Microsoft Research
	// The following algorithm also handles divisor of value 0 returning 0 for both quotient and remainder

	auto retType = llvm::VectorType::get(_type, 2);
	auto func = llvm::Function::Create(llvm::FunctionType::get(retType, {_type, _type}, false), llvm::Function::PrivateLinkage, _funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto zero = llvm::ConstantInt::get(_type, 0);
	auto one = llvm::ConstantInt::get(_type, 1);

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto yArg = x->getNextNode();
	yArg->setName("y");

	auto entryBB = llvm::BasicBlock::Create(_module.getContext(), "Entry", func);
	auto mainBB = llvm::BasicBlock::Create(_module.getContext(), "Main", func);
	auto loopBB = llvm::BasicBlock::Create(_module.getContext(), "Loop", func);
	auto continueBB = llvm::BasicBlock::Create(_module.getContext(), "Continue", func);
	auto returnBB = llvm::BasicBlock::Create(_module.getContext(), "Return", func);

	auto builder = IRBuilder{entryBB};
	auto yLEx = builder.CreateICmpULE(yArg, x);
	auto r0 = x;
	builder.CreateCondBr(yLEx, mainBB, returnBB);

	builder.SetInsertPoint(mainBB);
	auto ctlzIntr = llvm::Intrinsic::getDeclaration(&_module, llvm::Intrinsic::ctlz, _type);
	// both y and r are non-zero
	auto yLz = builder.CreateCall(ctlzIntr, {yArg, builder.getInt1(true)}, "y.lz");
	auto rLz = builder.CreateCall(ctlzIntr, {r0, builder.getInt1(true)}, "r.lz");
	auto i0 = builder.CreateNUWSub(yLz, rLz, "i0");
	auto y0 = builder.CreateShl(yArg, i0);
	builder.CreateBr(loopBB);

	builder.SetInsertPoint(loopBB);
	auto yPhi = builder.CreatePHI(_type, 2, "y.phi");
	auto rPhi = builder.CreatePHI(_type, 2, "r.phi");
	auto iPhi = builder.CreatePHI(_type, 2, "i.phi");
	auto qPhi = builder.CreatePHI(_type, 2, "q.phi");
	auto rUpdate = builder.CreateNUWSub(rPhi, yPhi);
	auto qUpdate = builder.CreateOr(qPhi, one);	// q += 1, q lowest bit is 0
	auto rGEy = builder.CreateICmpUGE(rPhi, yPhi);
	auto r1 = builder.CreateSelect(rGEy, rUpdate, rPhi, "r1");
	auto q1 = builder.CreateSelect(rGEy, qUpdate, qPhi, "q");
	auto iZero = builder.CreateICmpEQ(iPhi, zero);
	builder.CreateCondBr(iZero, returnBB, continueBB);

	builder.SetInsertPoint(continueBB);
	auto i2 = builder.CreateNUWSub(iPhi, one);
	auto q2 = builder.CreateShl(q1, one);
	auto y2 = builder.CreateLShr(yPhi, one);
	builder.CreateBr(loopBB);

	yPhi->addIncoming(y0, mainBB);
	yPhi->addIncoming(y2, continueBB);
	rPhi->addIncoming(r0, mainBB);
	rPhi->addIncoming(r1, continueBB);
	iPhi->addIncoming(i0, mainBB);
	iPhi->addIncoming(i2, continueBB);
	qPhi->addIncoming(zero, mainBB);
	qPhi->addIncoming(q2, continueBB);

	builder.SetInsertPoint(returnBB);
	auto qRet = builder.CreatePHI(_type, 2, "q.ret");
	qRet->addIncoming(zero, entryBB);
	qRet->addIncoming(q1, loopBB);
	auto rRet = builder.CreatePHI(_type, 2, "r.ret");
	rRet->addIncoming(r0, entryBB);
	rRet->addIncoming(r1, loopBB);
	auto ret = builder.CreateInsertElement(llvm::UndefValue::get(retType), qRet, uint64_t(0), "ret0");
	ret = builder.CreateInsertElement(ret, rRet, 1, "ret");
	builder.CreateRet(ret);

	return func;
}
}

llvm::Function* Arith256::getUDivRem256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.udivrem.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	return createUDivRemFunc(Type::Word, _module, funcName);
}

llvm::Function* Arith256::getUDivRem512Func(llvm::Module& _module)
{
	static const auto funcName = "evm.udivrem.i512";
	if (auto func = _module.getFunction(funcName))
		return func;

	return createUDivRemFunc(llvm::IntegerType::get(_module.getContext(), 512), _module, funcName);
}

llvm::Function* Arith256::getUDiv256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.udiv.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	auto udivremFunc = getUDivRem256Func(_module);

	auto func = llvm::Function::Create(llvm::FunctionType::get(Type::Word, {Type::Word, Type::Word}, false), llvm::Function::PrivateLinkage, funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto bb = llvm::BasicBlock::Create(_module.getContext(), {}, func);
	auto builder = IRBuilder{bb};
	auto udivrem = builder.CreateCall(udivremFunc, {x, y});
	auto udiv = builder.CreateExtractElement(udivrem, uint64_t(0));
	builder.CreateRet(udiv);

	return func;
}

namespace
{
llvm::Function* createURemFunc(llvm::Type* _type, llvm::Module& _module, char const* _funcName)
{
	auto udivremFunc = _type == Type::Word ? Arith256::getUDivRem256Func(_module) : Arith256::getUDivRem512Func(_module);

	auto func = llvm::Function::Create(llvm::FunctionType::get(_type, {_type, _type}, false), llvm::Function::PrivateLinkage, _funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto bb = llvm::BasicBlock::Create(_module.getContext(), {}, func);
	auto builder = IRBuilder{bb};
	auto udivrem = builder.CreateCall(udivremFunc, {x, y});
	auto r = builder.CreateExtractElement(udivrem, uint64_t(1));
	builder.CreateRet(r);

	return func;
}
}

llvm::Function* Arith256::getURem256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.urem.i256";
	if (auto func = _module.getFunction(funcName))
		return func;
	return createURemFunc(Type::Word, _module, funcName);
}

llvm::Function* Arith256::getURem512Func(llvm::Module& _module)
{
	static const auto funcName = "evm.urem.i512";
	if (auto func = _module.getFunction(funcName))
		return func;
	return createURemFunc(llvm::IntegerType::get(_module.getContext(), 512), _module, funcName);
}

llvm::Function* Arith256::getSDivRem256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.sdivrem.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	auto udivremFunc = getUDivRem256Func(_module);

	auto retType = llvm::VectorType::get(Type::Word, 2);
	auto func = llvm::Function::Create(llvm::FunctionType::get(retType, {Type::Word, Type::Word}, false), llvm::Function::PrivateLinkage, funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto bb = llvm::BasicBlock::Create(_module.getContext(), "", func);
	auto builder = IRBuilder{bb};
	auto xIsNeg = builder.CreateICmpSLT(x, Constant::get(0));
	auto xNeg = builder.CreateSub(Constant::get(0), x);
	auto xAbs = builder.CreateSelect(xIsNeg, xNeg, x);

	auto yIsNeg = builder.CreateICmpSLT(y, Constant::get(0));
	auto yNeg = builder.CreateSub(Constant::get(0), y);
	auto yAbs = builder.CreateSelect(yIsNeg, yNeg, y);

	auto res = builder.CreateCall(udivremFunc, {xAbs, yAbs});
	auto qAbs = builder.CreateExtractElement(res, uint64_t(0));
	auto rAbs = builder.CreateExtractElement(res, 1);

	// the remainder has the same sign as dividend
	auto rNeg = builder.CreateSub(Constant::get(0), rAbs);
	auto r = builder.CreateSelect(xIsNeg, rNeg, rAbs);

	auto qNeg = builder.CreateSub(Constant::get(0), qAbs);
	auto xyOpposite = builder.CreateXor(xIsNeg, yIsNeg);
	auto q = builder.CreateSelect(xyOpposite, qNeg, qAbs);

	auto ret = builder.CreateInsertElement(llvm::UndefValue::get(retType), q, uint64_t(0));
	ret = builder.CreateInsertElement(ret, r, 1);
	builder.CreateRet(ret);

	return func;
}

llvm::Function* Arith256::getSDiv256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.sdiv.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	auto sdivremFunc = getSDivRem256Func(_module);

	auto func = llvm::Function::Create(llvm::FunctionType::get(Type::Word, {Type::Word, Type::Word}, false), llvm::Function::PrivateLinkage, funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto bb = llvm::BasicBlock::Create(_module.getContext(), {}, func);
	auto builder = IRBuilder{bb};
	auto sdivrem = builder.CreateCall(sdivremFunc, {x, y});
	auto q = builder.CreateExtractElement(sdivrem, uint64_t(0));
	builder.CreateRet(q);

	return func;
}

llvm::Function* Arith256::getSRem256Func(llvm::Module& _module)
{
	static const auto funcName = "evm.srem.i256";
	if (auto func = _module.getFunction(funcName))
		return func;

	auto sdivremFunc = getSDivRem256Func(_module);

	auto func = llvm::Function::Create(llvm::FunctionType::get(Type::Word, {Type::Word, Type::Word}, false), llvm::Function::PrivateLinkage, funcName, &_module);
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory();

	auto x = &func->getArgumentList().front();
	x->setName("x");
	auto y = x->getNextNode();
	y->setName("y");

	auto bb = llvm::BasicBlock::Create(_module.getContext(), {}, func);
	auto builder = IRBuilder{bb};
	auto sdivrem = builder.CreateCall(sdivremFunc, {x, y});
	auto r = builder.CreateExtractElement(sdivrem, uint64_t(1));
	builder.CreateRet(r);

	return func;
}

llvm::Function* Arith256::getExpFunc()
{
	if (!m_exp)
	{
		llvm::Type* argTypes[] = {Type::Word, Type::Word};
		m_exp = llvm::Function::Create(llvm::FunctionType::get(Type::Word, argTypes, false), llvm::Function::PrivateLinkage, "exp", getModule());
		m_exp->setDoesNotThrow();
		m_exp->setDoesNotAccessMemory();

		auto base = &m_exp->getArgumentList().front();
		base->setName("base");
		auto exponent = base->getNextNode();
		exponent->setName("exponent");

		InsertPointGuard guard{m_builder};

		//	while (e != 0) {
		//		if (e % 2 == 1)
		//			r *= b;
		//		b *= b;
		//		e /= 2;
		//	}

		auto entryBB = llvm::BasicBlock::Create(m_builder.getContext(), "Entry", m_exp);
		auto headerBB = llvm::BasicBlock::Create(m_builder.getContext(), "LoopHeader", m_exp);
		auto bodyBB = llvm::BasicBlock::Create(m_builder.getContext(), "LoopBody", m_exp);
		auto updateBB = llvm::BasicBlock::Create(m_builder.getContext(), "ResultUpdate", m_exp);
		auto continueBB = llvm::BasicBlock::Create(m_builder.getContext(), "Continue", m_exp);
		auto returnBB = llvm::BasicBlock::Create(m_builder.getContext(), "Return", m_exp);

		m_builder.SetInsertPoint(entryBB);
		m_builder.CreateBr(headerBB);

		m_builder.SetInsertPoint(headerBB);
		auto r = m_builder.CreatePHI(Type::Word, 2, "r");
		auto b = m_builder.CreatePHI(Type::Word, 2, "b");
		auto e = m_builder.CreatePHI(Type::Word, 2, "e");
		auto eNonZero = m_builder.CreateICmpNE(e, Constant::get(0), "e.nonzero");
		m_builder.CreateCondBr(eNonZero, bodyBB, returnBB);

		m_builder.SetInsertPoint(bodyBB);
		auto eOdd = m_builder.CreateICmpNE(m_builder.CreateAnd(e, Constant::get(1)), Constant::get(0), "e.isodd");
		m_builder.CreateCondBr(eOdd, updateBB, continueBB);

		m_builder.SetInsertPoint(updateBB);
		auto mul256Func = getMulFunc(*getModule());
		auto r0 = m_builder.CreateCall(mul256Func, {r, b});
		m_builder.CreateBr(continueBB);

		m_builder.SetInsertPoint(continueBB);
		auto r1 = m_builder.CreatePHI(Type::Word, 2, "r1");
		r1->addIncoming(r, bodyBB);
		r1->addIncoming(r0, updateBB);
		auto b1 = m_builder.CreateCall(mul256Func, {b, b});
		auto e1 = m_builder.CreateLShr(e, Constant::get(1), "e1");
		m_builder.CreateBr(headerBB);

		r->addIncoming(Constant::get(1), entryBB);
		r->addIncoming(r1, continueBB);
		b->addIncoming(base, entryBB);
		b->addIncoming(b1, continueBB);
		e->addIncoming(exponent, entryBB);
		e->addIncoming(e1, continueBB);

		m_builder.SetInsertPoint(returnBB);
		m_builder.CreateRet(r);
	}
	return m_exp;
}

llvm::Value* Arith256::exp(llvm::Value* _arg1, llvm::Value* _arg2)
{
	//	while (e != 0) {
	//		if (e % 2 == 1)
	//			r *= b;
	//		b *= b;
	//		e /= 2;
	//	}

	if (auto c1 = llvm::dyn_cast<llvm::ConstantInt>(_arg1))
	{
		if (auto c2 = llvm::dyn_cast<llvm::ConstantInt>(_arg2))
		{
			auto b = c1->getValue();
			auto e = c2->getValue();
			auto r = llvm::APInt{256, 1};
			while (e != 0)
			{
				if (e[0])
					r *= b;
				b *= b;
				e = e.lshr(1);
			}
			return Constant::get(r);
		}
	}

	return m_builder.CreateCall(getExpFunc(), {_arg1, _arg2});
}

}
}
}

extern "C"
{
	EVMJIT_API void debug(uint64_t a, uint64_t b, uint64_t c, uint64_t d, char z)
	{
		DLOG(JIT) << "DEBUG " << std::dec << z << ": " //<< d << c << b << a
				<< " ["	<< std::hex << std::setfill('0') << std::setw(16) << d << std::setw(16) << c << std::setw(16) << b << std::setw(16) << a << "]\n";
	}
}
