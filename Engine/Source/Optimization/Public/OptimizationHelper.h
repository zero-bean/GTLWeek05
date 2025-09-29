#pragma once

namespace Optimization
{
	/* Primitive의 추가를 알아야 하는 다른 객체들에게 알린다. */
	void NotifyPrimitiveAdditionToOthers();

	/* Primitive의 삭제를 알아야 하는 다른 객체들에게 알린다. */
	void NotifyPrimitiveDeletionToOthers();

	/* Primitive의 변경 사항을 알아야 하는 다른 객체들에게 알린다. */
	void NotifyPrimitiveDirtyToOthers();
}